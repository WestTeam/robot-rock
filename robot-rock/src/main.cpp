// Copyright (c) 2018-2019 All Rights Reserved WestBot

#include <QCoreApplication>

#include <WestBot/HumanAfterAll/Category.hpp>
#include <WestBot/HumanAfterAll/ConsoleAppender.hpp>
#include <WestBot/HumanAfterAll/Handler.hpp>

//#define DEBUG
//#define SIMU
#define USE_SIMULATOR

#ifdef USE_SIMULATOR
#include <WestBot/RobotRock/SystemManagerSimu.hpp>
#else
#include <WestBot/RobotRock/Hal.hpp>
#include <WestBot/RobotRock/SystemManagerHw.hpp>
#endif

#include "StrategyManagerHomologation.hpp"
#include "StrategyManagerV1.hpp"
#include "StrategyManagerMatch1.hpp"


using namespace WestBot;
using namespace WestBot::RobotRock;

using namespace WestBot::HumanAfterAll::Logging;

namespace
{
    HUMANAFTERALL_LOGGING_CATEGORY( LOG, "WestBot.RobotRock.Main" )
}

int main( int argc, char *argv[] )
{
    QCoreApplication app( argc, argv );

    Handler handler( app.instance() );
    ConsoleAppender consoleAppender;
    handler.addAppender( & consoleAppender );

#ifdef DEBUG
    handler.setEnableDebugLevel( true );
#endif



    //StrategyManagerV1::Ptr strategyV1 =
    //    std::make_shared< StrategyManagerV1 >();

<<<<<<< Updated upstream
    StrategyManagerHomologation::Ptr strategyHomologation =
        std::make_shared< StrategyManagerHomologation >();

#ifndef USE_SIMULATOR
=======
#ifndef USE_SIMULATOR
    StrategyManagerMatch1::Ptr strategy =
        std::make_shared< StrategyManagerMatch1 >();
>>>>>>> Stashed changes
    Hal::Ptr hal = std::make_shared< Hal >();
    SystemManagerHw system( hal, strategy );

    tInfo( LOG ) << "==== System started ! ==== ";

    if( ! system.init() )
    {
        tFatal( LOG ) << "Failed to init system manager";
    }

  #ifdef DEBUG
    hal->dump();
  #endif

  #ifdef SIMU
    hal->_modeSimu.write( 1 );
  #endif
#else

    SystemManagerSimu system( strategyHomologation );

    tInfo( LOG ) << "==== System started ! ==== ";

    if( ! system.init() )
    {
        tFatal( LOG ) << "Failed to init system manager";
    }

#endif

    tInfo( LOG ) << "==== System ready ! ==== ";

    /*
    while(1)
    {
        app.processEvents();
    }*/

    return app.exec();
}
