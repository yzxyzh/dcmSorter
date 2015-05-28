cmake_minimum_required(VERSION 2.8)

project(dcmSorter)


set(moc_header dcmsorter.h)
set(source main.cpp dcmsorter.cpp myMacro.h)
set(ui dcmsorter.ui)

find_package(OpenCV REQUIRED)
find_package(ITK REQUIRED)
find_package(QT4 REQUIRED)

INCLUDE(${ITK_USE_FILE})
INCLUDE(${QT_USE_FILE})

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${PROJECT_SOURCE_DIR}/tixml)

add_definitions(-DTIXML_USE_STL)

file(GLOB tixmlSrc ${PROJECT_SOURCE_DIR}/tixml/*.cpp)

QT4_WRAP_CPP(MOCSrcs ${moc_header})
QT4_WRAP_UI(MOCUI ${ui})

message(${MOCUI})

add_executable(dcmSorter ${source} ${MOCSrcs} ${moc_header} ${MOCUI} ${tixmlSrc})

target_link_libraries(dcmSorter ${ITK_LIBRARIES} ${OpenCV_LIBS} ${QT_LIBRARIES})