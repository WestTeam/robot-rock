# Copyright (c) 2019 All Rights Reserved WestBot

QT += core network serialport

TARGET = robot-rock
SOURCES =  \
    src/StrategyManagerHomologation.cpp \
    src/StrategyManagerV1.cpp \
    src/StrategyManagerMatch1.cpp \
    src/StrategyManagerMatch2.cpp \
    src/StrategyManagerMatch3.cpp \
    src/StrategyManagerMatch4.cpp \
    src/StrategyManagerDebugPosition.cpp \
    src/main.cpp

HEADERS += \
    include/StrategyManagerHomologation.hpp \
    include/StrategyManagerV1.hpp \
    include/StrategyManagerMatch1.hpp \
    include/StrategyManagerMatch2.hpp \
    include/StrategyManagerMatch3.hpp \
    include/StrategyManagerMatch4.hpp \
    include/StrategyManagerDebugPosition.hpp


INCLUDEPATH += \
    include \
    ../3rdparty/HumanAfterAll/include/ \
    ../3rdparty/rplidar/include/ \
    ../3rdparty/a-star/include/
LIBS += -L../../robot-rock/libs -lHumanAfterAll -lRPLidar -lAStar

INCLUDEPATH += ../core/include
LIBS += -L../core -lcore

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

target.path = /home/ichiro
INSTALLS += target
# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0
