cmake_minimum_required(VERSION 3.12)
project(NextStudio)


#set any of these to "ON" if you want to build one of the juce examples
#or extras (Projucer/AudioPluginHost, etc):
option(JUCE_BUILD_EXTRAS "Build JUCE Examples" OFF)
option(JUCE_BUILD_EXAMPLES "Build JUCE Extras" OFF)

#adding juce as a subdirectory:
add_subdirectory(modules/tracktion_engine/modules/juce)

juce_add_modules(modules/tracktion_engine/modules/tracktion_engine)
juce_add_modules(modules/tracktion_engine/modules/tracktion_graph)

#adding project folders:
add_subdirectory(App)



