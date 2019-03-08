// Copyright (c) 2018-2019 All Rights Reserved WestBot

#include <QThread>

#include <Macros.hpp>

#include <WestBot/HumanAfterAll/Category.hpp>

#include <WestBot/RobotRock/SystemManager.hpp>

#define NO_LIDAR

using namespace WestBot;
using namespace WestBot::RobotRock;

namespace
{
    HUMANAFTERALL_LOGGING_CATEGORY( LOG, "WestBot.RobotRock.SystemManager" )

    const int GAME_DURATION = 90 * 1000; // 90s
}

SystemManager::SystemManager( const Hal::Ptr& hal, QObject* parent )
    : QObject( parent )
    , _hal( hal )
    , _startButton(
          new Input(
              std::make_shared< ItemRegister >( _hal->_input0 ),
             "Tirette" ) )
    , _colorButton(
          new Input(
              std::make_shared< ItemRegister >( _hal->_input1 ),
              "Color" ) )
    , _hardstopButton(
          new Input(
              std::make_shared< ItemRegister >( _hal->_input2 ),
              "AU" ) )
    , _ledYellow(
          new Output(
              std::make_shared< ItemRegister >( _hal->_output0 ),
              "Yellow" ) )
    , _ledBlue(
          new Output(
              std::make_shared< ItemRegister >( _hal->_output2 ),
              "Blue" ) )
    , _color( Color::Unknown )
    , _recalage( std::make_shared< Recalage >() )
    , _lidar( _recalage )
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

            if( value == DigitalValue::OFF &&
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
    _hal->_pidDistanceEnable.write( 0 );
    _hal->_pidDistanceOverride.write( 0 );
    _hal->_pidDistanceInverted.write( 0 );
    _hal->_pidDistanceKp.write( ( float ) 2000.0 );
    _hal->_pidDistanceKi.write( ( float ) 0.0 );
    _hal->_pidDistanceKd.write( ( float ) 0.0 );

    // We set it but this is override by the TrajectoryManager
    _hal->_pidDistanceSpeed.write( ( float ) 0.01 );
    _hal->_pidDistanceAcceleration.write( ( float ) 0.0001 );
    _hal->_pidDistanceSaturation.write( 25000 );

    _hal->_pidDistanceTarget.write( _hal->_pidDistancePosition.read< float >() );
    _hal->_pidDistanceEnable.write( 1 );

    // Config PID Angle
    _hal->_pidAngleEnable.write( 0 );
    _hal->_pidAngleOverride.write( 0 );
    _hal->_pidAngleInverted.write( 1 );
    _hal->_pidAngleKp.write( ( float ) 500000.0 );
    _hal->_pidAngleKi.write( ( float ) 0.0 );
    _hal->_pidAngleKd.write( ( float ) 0.0 );

    _hal->_pidAngleSpeed.write( ( float ) 0.0001 );
    _hal->_pidAngleAcceleration.write( ( float ) 0.00000002 );
    _hal->_pidAngleSaturation.write( 25000 );

    _hal->_pidAngleTarget.write( _hal->_pidAnglePosition.read< float >() );
    _hal->_pidAngleEnable.write( 1 );

    if( ! _recalage->init( _hal ) )
    {
        tWarning( LOG ) << "Failed to init recalage module";
        return false;
    }

#ifndef NO_LIDAR

    if( ! _lidar.init() )
    {
        tWarning( LOG ) << "Failed to init lidar module";
        return false;
    }

#endif

    _trajectoryManager.init();

    // Override output registers
    _hal->_outputOverride.write( 0x01010101 );

    displayColor( _colorButton->digitalRead() );

    tInfo( LOG ) << "System manager initialized";

    return true;
}

void SystemManager::start()
{
    if( nullptr != _game && _game->isRunning() )
    {
        tWarning( LOG ) << "Game is already running";
        return;
    }

    tInfo( LOG ) << "System starting...";

    if( ! isSafe() )
    {
        tFatal( LOG )
            << "System not safe to start: Odometry check failed";
    }

#ifndef NO_LIDAR
    // This run the lidar thread for data acquisition.
    _lidar.start();
#endif

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

#ifdef NO_LIDAR
    if( _lidar.isRunning() )
    {
        _lidar.terminate();
        _lidar.stopScan();
    }
#endif

    if( nullptr != _game && _game->isRunning() )
    {
        _game->terminate();
    }

    _strategyManager.stop();

    tInfo( LOG ) << "System stopped";

    reset();
}

void SystemManager::reset()
{
    _hal->_colorEnable.write( 0 );

    _hal->_resetAll.write( 1 );

    _hal->clearRegisters();

    _hal->_resetAll.write( 0 );

    _hal->_colorEnable.write( 1 );

    tInfo( LOG ) << "System was reset";

    init();
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
    int16_t x = _hal->_odometryX.read< int16_t >();
    int16_t y = _hal->_odometryY.read< int16_t >();
    int16_t theta = _hal->_odometryTheta.read< int16_t >();

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
        _recalage->errorInit( 0, 0, 0 );
    }
    else
    {
        _recalage->errorInit( 0, 0, 0 );
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