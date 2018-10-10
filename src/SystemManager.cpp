// Copyright (c) 2018 All Rights Reserved WestBot

#include <cstdio>

#include <QThread>

#include <Macros.hpp>

#include <WestBot/HumanAfterAll/Category.hpp>

#include <WestBot/RobotRock/SystemManager.hpp>

#define RPLIDAR_RESP_MEASUREMENT_SYNCBIT        (0x1<<0)
#define RPLIDAR_RESP_MEASUREMENT_ANGLE_SHIFT 1

#ifndef _countof
#define _countof(_Array) (int)(sizeof(_Array) / sizeof(_Array[0]))
#endif

using namespace WestBot;
using namespace WestBot::RobotRock;

namespace
{
    HUMANAFTERALL_LOGGING_CATEGORY( LOG, "WestBot.RobotRock.SystemManager" )

    const int GAME_DURATION = 90 * 1000; // 90s

    void plot_histogram( RPLidar::measurementNode_t* nodes, size_t count )
    {
        const int BARCOUNT =  75;
        const int MAXBARHEIGHT = 20;
        const float ANGLESCALE = 360.0f/BARCOUNT;

        qDebug() << "Plot histogramme data";

        float histogram[BARCOUNT];
        for (int pos = 0; pos < _countof(histogram); ++pos) {
            histogram[pos] = 0.0f;
        }

        float max_val = 0;
        for (int pos =0 ; pos < (int)count; ++pos) {
            int int_deg = (int)((nodes[pos].angle_q6_checkbit >> RPLIDAR_RESP_MEASUREMENT_ANGLE_SHIFT)/64.0f/ANGLESCALE);
            if (int_deg >= BARCOUNT) int_deg = 0;
            float cachedd = histogram[int_deg];
            if (cachedd == 0.0f ) {
                cachedd = nodes[pos].distance_q2/4.0f;
            } else {
                cachedd = (nodes[pos].distance_q2/4.0f + cachedd)/2.0f;
            }

            if (cachedd > max_val) max_val = cachedd;
            histogram[int_deg] = cachedd;
        }

        for (int height = 0; height < MAXBARHEIGHT; ++height) {
            float threshold_h = (MAXBARHEIGHT - height - 1) * (max_val/MAXBARHEIGHT);
            for (int xpos = 0; xpos < BARCOUNT; ++xpos) {
                if (histogram[xpos] >= threshold_h) {
                    putc('*', stdout);
                }else {
                    putc(' ', stdout);
                }
            }
            printf("\n");
        }
        for (int xpos = 0; xpos < BARCOUNT; ++xpos) {
            putc('-', stdout);
        }
        printf("\n");
    }

    bool capture_and_display( RPLidar::RPLidar& lidar )
    {
        bool ans = false;

        RPLidar::measurementNode_t nodes[ 8192 ];
        size_t   count = _countof(nodes);

        qDebug() << "waiting for data...\n";

        // fetech extactly one 0-360 degrees' scan
        ans = lidar.grabScanData(nodes, count);
        if( ans )
        {
            qDebug() << "Grabing scan data: OK";
            lidar.ascendScanData( nodes, count );
            plot_histogram(nodes, count);
        }
        else
        {
            qDebug() << "Error";
        }

        return ans;
    }
}

SystemManager::SystemManager( Hal& hal, QObject* parent )
    : QObject( parent )
    , _hal( hal )
    , _startButton(
          new Input(
              std::make_shared< ItemRegister >( _hal._input0 ),
             "Tirette" ) )
    , _colorButton(
          new Input(
              std::make_shared< ItemRegister >( _hal._input1 ),
              "Color" ) )
    , _hardstopButton(
          new Input(
              std::make_shared< ItemRegister >( _hal._input2 ),
              "AU" ) )
    , _ledYellow(
          new Output(
              std::make_shared< ItemRegister >( _hal._output0 ),
              "Yellow" ) )
    , _ledBlue(
          new Output(
              std::make_shared< ItemRegister >( _hal._output2 ),
              "Blue" ) )
    , _color( Color::Unknown )
    , _lidar( "/dev/ttyUSB0" )
    , _trajectoryManager( _hal, _recalage )
    , _systemMode( SystemManager::SystemMode::Full )
    , _strategyManager( _trajectoryManager )
    , _game( nullptr )
{
    connect(
        & _gameTimer,
        & QTimer::timeout,
        this,
        [ this ]()
        {
            tInfo( LOG ) << "Game ended";
            stop();
        } );

    // Task to notify that the robot is alive
    connect(
        & _aliveTimer,
        & QTimer::timeout,
        this,
        & SystemManager::robotAlive );

    connect(
        _startButton.get(),
        & Input::stateChanged,
        this,
        [ this ]( const DigitalValue& value )
        {
            tDebug( LOG ) << "Start button changed to:" << value;

            if( value == DigitalValue::ON &&
                _hardstopButton->digitalRead() == DigitalValue::OFF )
            {
                start();
            }
        } );

    connect(
        _colorButton.get(),
        & Input::stateChanged,
        this,
        [ this ]( const DigitalValue& value )
        {
            tDebug( LOG ) << "Color button changed to:" << value;
            displayColor( value );
        } );

    connect(
        _hardstopButton.get(),
        & Input::stateChanged,
        this,
        [ this ]( const DigitalValue& value )
        {
            tDebug( LOG ) << "Hardstop button changed to:" << value;

            if( value == DigitalValue::ON )
            {
                tInfo( LOG ) << "Hardstop requested";
                stop();
            }
        } );

    reset();
}

SystemManager::~SystemManager()
{
    stop();
}

//
// Public methods
//

bool SystemManager::init()
{
    tInfo( LOG ) << "System manager initializing...";

    // Config PID Distance
    _hal._pidDistanceEnable.write( 0 );
    _hal._pidDistanceOverride.write( 0 );
    _hal._pidDistanceInverted.write( 0 );
    _hal._pidDistanceKp.write( ( float ) 2000.0 );
    _hal._pidDistanceKi.write( ( float ) 0.0 );
    _hal._pidDistanceKd.write( ( float ) 0.0 );

    _hal._pidDistanceSpeed.write( ( float ) 0.01 );
    _hal._pidDistanceAcceleration.write( ( float ) 0.0001 );
    _hal._pidDistanceSaturation.write( 25000 );

    _hal._pidDistanceTarget.write( _hal._pidDistancePosition.read< float >() );
    _hal._pidDistanceEnable.write( 1 );

    // Config PID Angle
    _hal._pidAngleEnable.write( 0 );
    _hal._pidAngleOverride.write( 0 );
    _hal._pidAngleInverted.write( 1 );
    _hal._pidAngleKp.write( ( float ) 500000.0 );
    _hal._pidAngleKi.write( ( float ) 0.0 );
    _hal._pidAngleKd.write( ( float ) 0.0 );

    _hal._pidAngleSpeed.write( ( float ) 0.0001 );
    _hal._pidAngleAcceleration.write( ( float ) 0.00000002 );
    _hal._pidAngleSaturation.write( 25000 );

    _hal._pidAngleTarget.write( _hal._pidAnglePosition.read< float >() );
    _hal._pidAngleEnable.write( 1 );

    if( ! _lidar.connect() )
    {
       tWarning( LOG ) << "Cannot connect to RPLidar";
       return false;
    }

    QString lidarInfo = _lidar.getDeviceInfo();
    tDebug( LOG ).noquote() << lidarInfo;

    // check RPLidar health...
    if( ! _lidar.checkHealth() )
    {
        tWarning( LOG ) << "RPLidar not healthy";
        return false;
    }

    _lidar.startMotor();
    _lidar.startScan();

    capture_and_display( _lidar );

    _lidar.stopMotor();

    if( ! _recalage.init( _hal ) )
    {
        tWarning( LOG ) << "Failed to init recalage module";
        return false;
    }

    _trajectoryManager.init();

    // Override output registers
    _hal._outputOverride.write( 0x01010101 );

    displayColor( _colorButton->digitalRead() );

    tInfo( LOG ) << "System manager initialized";

    return true;
}

void SystemManager::start()
{
    tInfo( LOG ) << "System starting...";

    if( ! isSafe() )
    {
        tFatal( LOG )
            << "System not safe to start: Odometry check failed";
    }

    _lidar.startMotor();

    _lidar.startScan();

    QThread::sleep( 2 );

    _lidar.stopMotor();

    initRecalage();

    displayColor( _colorButton->digitalRead() );

    blinkColorLed();

    _gameTimer.start( GAME_DURATION );
    _gameTimer.setSingleShot( true );

    _game.reset( new GameThread( _strategyManager, _color ) );
    _game->start();
}

void SystemManager::stop()
{
    _gameTimer.stop();
    _aliveTimer.stop();

    _game->terminate();
    _strategyManager.stop();

    tInfo( LOG ) << "System stopped";

    reset();
}

void SystemManager::reset()
{
    _hal._colorEnable.write( 0 );

    _hal._resetAll.write( 1 );

    _hal.clearRegisters();

    _hal._resetAll.write( 0 );

    _hal._colorEnable.write( 1 );

    tInfo( LOG ) << "System was reset";
}

void SystemManager::setMode( SystemManager::SystemMode mode )
{
    _systemMode = mode;
}

SystemManager::SystemMode SystemManager::mode() const
{
    return _systemMode;
}

bool SystemManager::isSafe() const
{
    // ODOMETRY check
    int16_t x = _hal._odometryX.read< int16_t >();
    int16_t y = _hal._odometryY.read< int16_t >();
    int16_t theta = _hal._odometryTheta.read< int16_t >();

    tDebug( LOG ) << "X:" << x << " Y:" << y << " Theta:" << theta;

    int16_t safe = x + y + theta;

    if( safe != 0 )
    {
        return false;
    }

    return true;
}

//
// Private methods
//
void SystemManager::initRecalage()
{
    if( _color == Color::Yellow )
    {
        _recalage.errorInit( 36, 580, 0 ); // TODO: Change y pos
    }
    else
    {
        _recalage.errorInit( 36, -610, 0 ); // TODO: Change y pos
    }

    tInfo( LOG ) << "Odometry initialized for color:" << _color;
}

void SystemManager::blinkColorLed()
{
    _aliveTimer.start( 250 );
}

void SystemManager::robotAlive()
{
    if( _color == Color::Blue )
    {
        _ledBlue->digitalWrite( DigitalValue::OFF );
        QThread::msleep( 250 );
        _ledBlue->digitalWrite( DigitalValue::ON );
        QThread::msleep( 250 );
    }
    else
    {
        _ledYellow->digitalWrite( DigitalValue::OFF );
        QThread::msleep( 250 );
        _ledYellow->digitalWrite( DigitalValue::ON );
        QThread::msleep( 250 );
    }
}

void SystemManager::displayColor( const DigitalValue& value )
{
    if( value == DigitalValue::OFF )
    {
        _color = Color::Blue;
        _ledBlue->digitalWrite( DigitalValue::ON );
        _ledYellow->digitalWrite( DigitalValue::OFF );
    }
    else
    {
        _color = Color::Yellow;
        _ledBlue->digitalWrite( DigitalValue::OFF );
        _ledYellow->digitalWrite( DigitalValue::ON );
    }
}
