// Copyright (c) 2019 All Rights Reserved WestBot

#include <QThread>

#include <WestBot/HumanAfterAll/Category.hpp>

#include <WestBot/RobotRock/MoveAction.hpp>
#include <WestBot/RobotRock/StrategyManagerV1.hpp>
#include <WestBot/RobotRock/SystemManager.hpp>
#include <WestBot/RobotRock/WaitAction.hpp>

using namespace WestBot;
using namespace WestBot::RobotRock;

namespace
{
    HUMANAFTERALL_LOGGING_CATEGORY(
        LOG,
        "WestBot.RobotRock.StrategyManagerV1" )
}

StrategyManagerV1::StrategyManagerV1()
    : _trajectoryManager( nullptr )
    , _currentAction( nullptr )
    , _stratIsRunning( false )
    , _obstacleToClose( false )
    , _init( false )
{
}

bool StrategyManagerV1::init( const TrajectoryManager::Ptr& trajectoryManager )
{
    tDebug( LOG ) << "HERE 3";

    if ( ! _init )
    {
        _trajectoryManager = trajectoryManager;
        _init = true;
    }
    else
    {
        tDebug( LOG ) << "StrategyManagerV1 already initialized";
    }

    return true;
}

void StrategyManagerV1::deinit()
{
    _trajectoryManager = nullptr;
    _init = false;
}

void StrategyManagerV1::stop()
{
    hardStop();

    // Stop traj
    _trajectoryManager->hardStop();
    _trajectoryManager->disable();
}

// Private methods
void StrategyManagerV1::buildStrat( const Color& color )
{
    float inv = 1.0;
    float shift = 0.0;
    float offset = 0.0;

    if( color == Color::Yellow )
    {
        inv = -1.0;
        shift = 0.0;
        offset = 0.0;
    }
    else
    {
        inv = 1.0;
        shift = 0.0;
        offset = 0.0;
    }

    WaitAction::Ptr wait500Ms =
        std::make_shared< WaitAction >( 500 );

    WaitAction::Ptr wait200Ms =
        std::make_shared< WaitAction >( 200 );

    WaitAction::Ptr wait100Ms =
        std::make_shared< WaitAction >( 100 );

    // >>>>>>>>>>>>> MOVE ACTIONS
    MoveAction::Ptr moveToCenterZone =
        std::make_shared< MoveAction >(
            _trajectoryManager,
            TrajectoryManager::TrajectoryType::TYPE_TRAJ_GOTO_FORWARD_XY_ABS,
            0.0,
            0.0,
            inv * 500,
            600,
            true );

    MoveAction::Ptr moveToStartZone =
        std::make_shared< MoveAction >(
            _trajectoryManager,
            TrajectoryManager::TrajectoryType::TYPE_TRAJ_GOTO_FORWARD_XY_ABS,
            0.0,
            0.0,
            inv * 800,
            600,
            true );

    MoveAction::Ptr moveSmall =
        std::make_shared< MoveAction >(
            _trajectoryManager,
            TrajectoryManager::TrajectoryType::TYPE_TRAJ_D_REL,
            0.0,
            50.0,
            0.0,
            0.0,
            true );

    MoveAction::Ptr moveALittleForward =
        std::make_shared< MoveAction >(
            _trajectoryManager,
            TrajectoryManager::TrajectoryType::TYPE_TRAJ_D_REL,
            0.0,
            100.0,
            0.0,
            0.0,
            true );

    MoveAction::Ptr moveALittleForward2 =
        std::make_shared< MoveAction >(
            _trajectoryManager,
            TrajectoryManager::TrajectoryType::TYPE_TRAJ_D_REL,
            0.0,
            150.0,
            0.0,
            0.0,
            true );

    MoveAction::Ptr turnA90 =
            std::make_shared< MoveAction >(
                _trajectoryManager,
                TrajectoryManager::TrajectoryType::TYPE_TRAJ_ONLY_A_ABS,
                90.0,
                0.0,
                0.0,
                0.0,
                true );

    MoveAction::Ptr turnRedZone =
            std::make_shared< MoveAction >(
                _trajectoryManager,
                TrajectoryManager::TrajectoryType::TYPE_TRAJ_A_REL,
                20.0,
                0.0,
                0.0,
                0.0,
                true );

    MoveAction::Ptr orientationZoneDepose =
            std::make_shared< MoveAction >(
                _trajectoryManager,
                TrajectoryManager::TrajectoryType::TYPE_TRAJ_A_REL,
                20.0 * inv,
                0.0,
                0.0,
                0.0,
                true );

    _actions.push_back( wait500Ms );
    _actions.push_back( wait500Ms );
    _actions.push_back( wait500Ms );

    _actions.push_back( moveToCenterZone );
    _actions.push_back( wait500Ms );
    _actions.push_back( wait500Ms );
    _actions.push_back( wait500Ms );

    _actions.push_back( turnA90 );
    _actions.push_back( wait500Ms );
    _actions.push_back( wait500Ms );
    _actions.push_back( wait500Ms );

    _actions.push_back( moveALittleForward );
    _actions.push_back( wait500Ms );
    _actions.push_back( wait500Ms );
    _actions.push_back( wait500Ms );

    _actions.push_back( moveALittleForward2 );
    _actions.push_back( wait500Ms );
    _actions.push_back( wait500Ms );
    _actions.push_back( wait500Ms );

    _actions.push_back( moveToStartZone );
    _actions.push_back( wait500Ms );
    _actions.push_back( wait500Ms );
    _actions.push_back( wait500Ms );

    _actions.push_back( orientationZoneDepose );
    _actions.push_back( wait500Ms );
    _actions.push_back( wait500Ms );
    _actions.push_back( wait500Ms );

    _actions.push_back( moveALittleForward2 );
    _actions.push_back( wait500Ms );
    _actions.push_back( wait500Ms );
    _actions.push_back( wait500Ms );


    _stratIsRunning = true;
    _trajectoryManager->setAbort( false );
}

void StrategyManagerV1::doStrat( const Color& color )
{
	// Build the strat for selected color
    buildStrat( color );

    tDebug( LOG ) << "Do strat for color:" << color;

    WaitAction::Ptr wait500ms =
        std::make_shared< WaitAction >( 500 );

    while( _stratIsRunning )
    {
        if( _obstacleToClose )
        {
            _trajectoryManager->hardStop();
            _trajectoryManager->setAbort(false);

            _actions.push_front( wait500ms );
        }

        if( _actions.size() != 0 )
        {
            _currentAction = _actions.takeFirst();
            _currentAction->execute();
        }
        else {
            tDebug( LOG ) << "NO MORE ACTIONS: Wait until the end";
            _actions.push_front( wait500ms );
        }
    }
}

void StrategyManagerV1::hardStop()
{
    _trajectoryManager->setAbort( true );
    _stratIsRunning = false;
    _actions.clear();
}

void StrategyManagerV1::obstacleToClose( bool avoid )
{
    _obstacleToClose = avoid;

    if( _obstacleToClose )
    {
        _trajectoryManager->setAbort( true );
    }
}
