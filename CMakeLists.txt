cmake_minimum_required(VERSION 3.12)
project(NextStudio)

#adding my own custom git functions, to save some boilerplate::
include (CMake/GitFunctions.cmake)

#calling my custom function to grab JUCE 6 from git:
update_from_git(juce "https://github.com/juce-framework/juce" master)
update_from_git(tracktion_engine "https://github.com/Tracktion/tracktion_engine.git" develop)

#set any of these to "ON" if you want to build one of the juce examples
#or extras (Projucer/AudioPluginHost, etc):
option(JUCE_BUILD_EXTRAS "Build JUCE Examples" OFF)
option(JUCE_BUILD_EXAMPLES "Build JUCE Extras" OFF)

#adding juce as a subdirectory:
add_subdirectory(${juce_SOURCE_DIR})

juce_add_modules(${tracktion_engine_SOURCE_DIR}/modules/tracktion_engine)
#juce_add_modules(${tracktion_engine_SOURCE_DIR}/modules/tracktion_graph)

#adding project folders:
add_subdirectory(App)



