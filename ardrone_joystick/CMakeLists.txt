cmake_minimum_required(VERSION 2.4.6)
project(ardrone_joystick)

find_package(catkin REQUIRED COMPONENTS
        geometry_msgs
        std_srvs
        roscpp
        sensor_msgs
        cmake_modules
        )

include_directories(
        include
        ${catkin_INCLUDE_DIRS}
)

catkin_package(
        CATKIN_DEPENDS geometry_msgs std_srvs roscpp sensor_msgs
)


add_executable(ardrone_teleop src/main.cpp)
target_link_libraries(ardrone_teleop ${catkin_LIBRARIES})
add_dependencies(ardrone_teleop ${catkin_EXPORTED_TARGETS})