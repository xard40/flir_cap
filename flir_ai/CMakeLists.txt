############### 指定 cmake的最小版本  ###################################333
cmake_minimum_required (VERSION 3.16)

project ( sAIWays VERSION 0.0.1 LANGUAGES C CXX)
#============================================================
find_file (CONFIG_PATH config ${CMAKE_CURRENT_SOURCE_DIR})
set (CMAKE_CXX_STANDARD_REQUIRED ON)
add_definitions (-std=c++17
                 -pthread
                 -O3
                 -DCONFIG_YAML_DIR=${CONFIG_PATH}
)
# Add a compiler flag
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
#-----------------------------------------------------------------
set(CMAKE_BUILD_TYPE "RELEASE")
# ================= Standard =============================
set (CMAKE_AUTOMOC ON)
set (CMAKE_AUTORCC ON)
set (CMAKE_AUTOUIC ON)
set (CMAKE_AUTOUIC_SEARCH_PATHS ui)

#========  set app and utils directories  =======================
set (executable_name goFLIR)

# =================  CUDA  =============================
find_package (CUDA REQUIRED)
    message(STATUS "Found CUDA ${CUDA_VERSION_STRING} at ${CUDA_TOOLKIT_ROOT_DIR}")
    message(STATUS "Find CUDA include at ${CUDA_INCLUDE_DIRS}")
    message(STATUS "Find CUDA libraries: ${CUDA_LIBRARIES}")

# =================  TensorRT  ===========================
set (TENSORRT_ROOT /usr/src/tensorrt/)
find_path (TENSORRT_INCLUDE_DIR NvInfer.h
           HINTS ${TENSORRT_ROOT} PATH_SUFFIXES include/)
    message(STATUS "Found TensorRT headers at ${TENSORRT_INCLUDE_DIR}")
find_library (TENSORRT_LIBRARY_INFER nvinfer
              HINTS ${TENSORRT_ROOT} ${TENSORRT_BUILD} ${CUDA_TOOLKIT_ROOT_DIR}
              PATH_SUFFIXES lib lib64 lib/x64)
find_library (TENSORRT_LIBRARY_ONNXPARSER nvonnxparser
              HINTS  ${TENSORRT_ROOT} ${TENSORRT_BUILD} ${CUDA_TOOLKIT_ROOT_DIR}
              PATH_SUFFIXES lib lib64 lib/x64)
set (TENSORRT_LIBRARY ${TENSORRT_LIBRARY_INFER} ${TENSORRT_LIBRARY_ONNXPARSER})
    message(STATUS "Find TensorRT libs: ${TENSORRT_LIBRARY}")

# =================  OpenCV ===============================
find_package (OpenCV REQUIRED)
    message(STATUS "Find OpenCV include at ${OpenCV_INCLUDE_DIRS}")
    message(STATUS "Find OpenCV libraries: ${OpenCV_LIBRARIES}")

# #================= ffmpeg  ==============================
# find_package(PkgConfig REQUIRED)
# if (FFMPEG_FOUND)
# #  FFMPEG_INCLUDE_DIRS  - Include directory necessary for using the required components headers.
# #  FFMPEG_LIBRARIES     - Link these to use the required ffmpeg components.
# #  FFMPEG_DEFINITIONS   - Compiler switches required for using the required ffmpeg components.
#     message("FFMPEG_INCLUDE_DIRS = ${FFMPEG_INCLUDE_DIRS} ")
#     message("FFMPEG_LIBRARIES = ${FFMPEG_LIBRARIES} ")
#     message("FFMPEG_DEFINITIONS = ${FFMPEG_DEFINITIONS} ")
# endif()

# pkg_check_modules(LIBAV REQUIRED IMPORTED_TARGET
#     libavdevice
#     libavfilter
#     libavformat
#     libavcodec
#     libswresample
#     libswscale
#     libavutil
# )

#==============  Qt  =========================
#find_package(Qt6 COMPONENTS Core Widgets Gui)
#if (NOT Qt6_FOUND)
    # find_package(Qt5 5.12 REQUIRED COMPONENTS Core Widgets Gui)
#--------------------------------------------------------------
#find_package (Qt5 REQUIRED COMPONENTS Widgets)
#find_package (Qt5 REQUIRED COMPONENTS Gui)
#find_package (Qt5 REQUIRED COMPONENTS Core)
#endif()

# ==============  Spinnaker SDK  ====================
if (WIN32)
    set (Spinnaker_LIBS debug "Spinnakerd_v140.lib" optimized "Spinnaker_v140.lib")
    set (Spinnaker_INCLUDE_DIR
        "C:/Program Files/FLIR Systems/Spinnaker/include")
    link_directories(
        "C:/Program Files/FLIR Systems/Spinnaker/lib64/vs2015")
endif(WIN32)
#-------------------------------
if (UNIX)
    add_definitions(-DLINUX)
    find_path(Spinnaker_INCLUDE_DIR Spinnaker.h
      PATHS /opt/spinnaker /usr /usr/local
      PATH_SUFFIXES include)
    find_library(Spinnaker_LIBS NAMES Spinnaker
      PATHS /opt/spinnaker /usr /usr/local
      PATH_SUFFIXES lib
)
endif(UNIX)

# ##############################################################################
#=========   set PROJECTMAIN PATH ===========================
set (PROJECTMAIN_PATH  ${CMAKE_CURRENT_SOURCE_DIR})
    message(STATUS "set  PROJECTMAIN_PATH: ${PROJECTMAIN_PATH}")
set (PROJECTMAIN_SRC
#---------------------------
    # ${PROJECTMAIN_PATH}/main.cpp
    ${PROJECTMAIN_PATH}/flirutils.cpp
    # ${PROJECTMAIN_PATH}/flirUtils.h
    # ${PROJECTMAIN_PATH}/captureDef.h
    # ${PROJECTMAIN_PATH}/appHandler.cpp
    # ${PROJECTMAIN_PATH}/appHandler.h
    # ${PROJECTMAIN_PATH}/flirHandler.h
    # ${PROJECTMAIN_PATH}/flirHandler.cpp
    # ${PROJECTMAIN_PATH}/callback/callbackHandler.h
    # ${PROJECTMAIN_PATH}/callback/callbackHandler.cpp
    # ${PROJECTMAIN_PATH}/sharedQueue.h
    # ${PROJECTMAIN_PATH}/spinManager.cpp
    # ${PROJECTMAIN_PATH}/spinManager.h
    # ${PROJECTMAIN_PATH}/spinCamera.cpp
    # ${PROJECTMAIN_PATH}/spinCamera.h
)

# #============  set Conveyor APP_PATH  =============================
# set (APPS_PATH  ${CMAKE_CURRENT_SOURCE_DIR}/rAIconveyorApp)
#     message(STATUS "set APPS_PATH: ${APPS_PATH}")
# set (APPS_SRC
#     ${APPS_PATH}/conveyorCtrl.h
#     ${APPS_PATH}/conveyorModel.h
#     ${APPS_PATH}/conveyorView.h
# #--------------------------------
#     ${APPS_PATH}/conveyorCtrl.cpp
#     ${APPS_PATH}/conveyorModel.cpp
#     ${APPS_PATH}/conveyorView.cpp
# )

# #============  set GRFWIDGET_PATH  =============================
# set (GRFWIDGET_PATH  ${CMAKE_CURRENT_SOURCE_DIR}/rAIconveyorApp/grfWidget)
#     message(STATUS "set GRFWIDGET_PATH: ${GRFWIDGET_PATH}")
# set (GRFWIDGET_SRC
#     ${GRFWIDGET_PATH}/grfView.h
#     ${GRFWIDGET_PATH}/grfFrame.h
#     ${GRFWIDGET_PATH}/grfListWidget.h
# #--------------------------------
#     ${GRFWIDGET_PATH}/grfView.cpp
#     ${GRFWIDGET_PATH}/grfFrame.cpp
#     ${GRFWIDGET_PATH}/grfListWidget.cpp
# )

# #===========  set MODELENGINE_PATH  =====================
# set (MODELENGINE_PATH  ${CMAKE_CURRENT_SOURCE_DIR}/modelEngine)
#     message(STATUS "set  MODELENGINE_PATH: ${MODELENGINE_PATH}")
# set (MODELENGINE_SRC
#       ${MODELENGINE_PATH}/engineDef.h
#       ${MODELENGINE_PATH}/conveyorMDEngine/conveyorMVEngine.h
#       ${MODELENGINE_PATH}/conveyorMDEngine/moveDetector.h
# #---------------------------------
#       ${MODELENGINE_PATH}/conveyorMDEngine/conveyorMVEngine.cpp
#       ${MODELENGINE_PATH}/conveyorMDEngine/moveDetector.cpp
# )

# #===========  set MGRUTILS_PATH  =====================
# set (MGRUTILS_PATH     ${CMAKE_CURRENT_SOURCE_DIR}/mgrUtils)
#     message(STATUS "set MGRUTILS_PATH: ${MGRUTILS_PATH}")
# set (MGRUTILS_SRC
#     ${MGRUTILS_PATH}/callbackHandler.h
#     ${MGRUTILS_PATH}/guiMenuDEF.h
#     ${MGRUTILS_PATH}/ribbonBarDEF.h
#     ${MGRUTILS_PATH}/sysConfig.h
#     ${MGRUTILS_PATH}/modelBase.h
#     ${MGRUTILS_PATH}/viewBase.h
#     ${MGRUTILS_PATH}/ctrlBase.h
#     ${MGRUTILS_PATH}/timer.h
#     ${MGRUTILS_PATH}/timerMgr.h
#     ${MGRUTILS_PATH}/msgLogger.h
# #----------------------------
#     ${MGRUTILS_PATH}/callbackHandler.cpp
#     ${MGRUTILS_PATH}/modelBase.cpp
#     ${MGRUTILS_PATH}/viewBase.cpp
#     ${MGRUTILS_PATH}/ctrlBase.cpp
#     ${MGRUTILS_PATH}/timer.cpp
#     ${MGRUTILS_PATH}/timerMgr.cpp
#     ${MGRUTILS_PATH}/msgLogger.cpp
# )

# #=========   set VIDEOCAPTURE PATH ===========================
# set (VIDEOCAPTURE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/videoCapture)
#     message(STATUS "set VIDEOCAPTURE_PATH: ${VIDEOCAPTURE_PATH}")
# set (VIDEOCAPTURE_SRC
#     ${VIDEOCAPTURE_PATH}/capHandler.h
#     ${VIDEOCAPTURE_PATH}/ffCapture.h
#     ${VIDEOCAPTURE_PATH}/captureDef.h
#     ${VIDEOCAPTURE_PATH}/exceptions.h
#     ${VIDEOCAPTURE_PATH}/avFrameDeque.h
#     ${VIDEOCAPTURE_PATH}/sharedQueue.h
# #---
#     ${VIDEOCAPTURE_PATH}/capHandler.cpp
#     ${VIDEOCAPTURE_PATH}/ffCapture.cpp
# )

# #=========   set IOTHANDLER_PATH ===========================
# set (IOTHANDLER_PATH   ${CMAKE_CURRENT_SOURCE_DIR}/ioHandler)
#     message(STATUS "set   IOTHANDLER_PATH: ${IOTHANDLER_PATH}")
# set (IOTHANDLER_SRC
#     ${IOTHANDLER_PATH}/modbusTCPDriver.h
# #----------------------------------
#     ${IOTHANDLER_PATH}/modbusTCPDriver.cpp
# )

# #=========   set FLIR_PATH ===========================
# set (FLIR_PATH   ${CMAKE_CURRENT_SOURCE_DIR})
#     message(STATUS "set   FLIR_PATH: ${FLIR_PATH}")
# set (FLIR_SRC
#     ${FLIR_PATH}/FLIRVision.h
#     ${FLIR_PATH}/FLIRVision.ui
# #----------------------------------
#     ${FLIR_PATH}/main.cpp
#     ${FLIR_PATH}/FLIRVision.cpp
# )

# #=========   set GUI SARIBBON PATH ===========================
# set (SARIBBON_PATH   ${CMAKE_CURRENT_SOURCE_DIR}/SARibbonBar)
#     message(STATUS "set SARIBBON_PATH: ${SARIBBON_PATH}")
# set (SARIBBON_SRC
#     ${SARIBBON_PATH}/SAFramelessHelper.h
#     ${SARIBBON_PATH}/SARibbonStyleOption.h
#     ${SARIBBON_PATH}/SARibbonActionsManager.h
#     ${SARIBBON_PATH}/SARibbonBar.h
#     ${SARIBBON_PATH}/SARibbonCustomizeData.h
#     ${SARIBBON_PATH}/SARibbonCustomizeDialog.h
#     ${SARIBBON_PATH}/SARibbonCustomizeWidget.h
#     ${SARIBBON_PATH}/SARibbonMainWindow.h
#     ${SARIBBON_PATH}/SAWindowButtonGroup.h
#     ${SARIBBON_PATH}/SARibbonApplicationButton.h
#     ${SARIBBON_PATH}/SARibbonTabBar.h
#     ${SARIBBON_PATH}/SARibbonCategory.h
#     ${SARIBBON_PATH}/SARibbonContextCategory.h
#     ${SARIBBON_PATH}/SARibbonPannel.h
#     ${SARIBBON_PATH}/SARibbonToolButton.h
#     ${SARIBBON_PATH}/SARibbonMenu.h
#     ${SARIBBON_PATH}/SARibbonGlobal.h
#     ${SARIBBON_PATH}/SARibbonPannelOptionButton.h
#     ${SARIBBON_PATH}/SARibbonSeparatorWidget.h
#     ${SARIBBON_PATH}/SARibbonCategoryLayout.h
#     ${SARIBBON_PATH}/SARibbonGallery.h
#     ${SARIBBON_PATH}/SARibbonControlButton.h
#     ${SARIBBON_PATH}/SARibbonGalleryGroup.h
#     ${SARIBBON_PATH}/SARibbonGalleryItem.h
#     ${SARIBBON_PATH}/SARibbonComboBox.h
#     ${SARIBBON_PATH}/SARibbonElementCreateDelegate.h
#     ${SARIBBON_PATH}/SARibbonElementManager.h
#     ${SARIBBON_PATH}/SARibbonDrawHelper.h
#     ${SARIBBON_PATH}/SARibbonLineEdit.h
#     ${SARIBBON_PATH}/SARibbonCheckBox.h
#     ${SARIBBON_PATH}/SARibbonButtonGroupWidget.h
#     ${SARIBBON_PATH}/SARibbonStackedWidget.h
#     ${SARIBBON_PATH}/SARibbonQuickAccessBar.h
#     ${SARIBBON_PATH}/SARibbonCtrlContainer.h
#     ${SARIBBON_PATH}/SARibbonPannelLayout.h
#     ${SARIBBON_PATH}/SARibbonPannelItem.h
#     ${SARIBBON_PATH}/SARibbonLineWidgetContainer.h
# #---------------------------------------------------
#     ${SARIBBON_PATH}/SAFramelessHelper.cpp
#     ${SARIBBON_PATH}/SARibbonBar.cpp
#     ${SARIBBON_PATH}/SARibbonMainWindow.cpp
#     ${SARIBBON_PATH}/SAWindowButtonGroup.cpp
#     ${SARIBBON_PATH}/SARibbonPannel.cpp
#     ${SARIBBON_PATH}/SARibbonTabBar.cpp
#     ${SARIBBON_PATH}/SARibbonCategory.cpp
#     ${SARIBBON_PATH}/SARibbonElementManager.cpp
#     ${SARIBBON_PATH}/SARibbonStyleOption.cpp
#     ${SARIBBON_PATH}/SARibbonActionsManager.cpp
#     ${SARIBBON_PATH}/SARibbonCustomizeData.cpp
#     ${SARIBBON_PATH}/SARibbonCustomizeDialog.cpp
#     ${SARIBBON_PATH}/SARibbonCustomizeWidget.cpp
#     ${SARIBBON_PATH}/SARibbonApplicationButton.cpp
#     ${SARIBBON_PATH}/SARibbonContextCategory.cpp
#     ${SARIBBON_PATH}/SARibbonToolButton.cpp
#     ${SARIBBON_PATH}/SARibbonMenu.cpp
#     ${SARIBBON_PATH}/SARibbonPannelOptionButton.cpp
#     ${SARIBBON_PATH}/SARibbonSeparatorWidget.cpp
#     ${SARIBBON_PATH}/SARibbonCategoryLayout.cpp
#     ${SARIBBON_PATH}/SARibbonGallery.cpp
#     ${SARIBBON_PATH}/SARibbonControlButton.cpp
#     ${SARIBBON_PATH}/SARibbonGalleryGroup.cpp
#     ${SARIBBON_PATH}/SARibbonGalleryItem.cpp
#     ${SARIBBON_PATH}/SARibbonComboBox.cpp
#     ${SARIBBON_PATH}/SARibbonElementCreateDelegate.cpp
#     ${SARIBBON_PATH}/SARibbonDrawHelper.cpp
#     ${SARIBBON_PATH}/SARibbonLineEdit.cpp
#     ${SARIBBON_PATH}/SARibbonCheckBox.cpp
#     ${SARIBBON_PATH}/SARibbonButtonGroupWidget.cpp
#     ${SARIBBON_PATH}/SARibbonStackedWidget.cpp
#     ${SARIBBON_PATH}/SARibbonQuickAccessBar.cpp
#     ${SARIBBON_PATH}/SARibbonCtrlContainer.cpp
#     ${SARIBBON_PATH}/SARibbonPannelLayout.cpp
#     ${SARIBBON_PATH}/SARibbonPannelItem.cpp
#     ${SARIBBON_PATH}/SARibbonLineWidgetContainer.cpp
# #--------------------------------------------------------
#     ${SARIBBON_PATH}/resource.qrc
# )

# #=========   set APPSQRCS PATH ===========================
# set (APPSQRCS_PATH   ${CMAKE_CURRENT_SOURCE_DIR}/resources/qrc)
#     message(STATUS "set APPSQRCS_PATH: ${APPSQRCS_PATH}")

#==================================================================

# ============ set include path 啟用當前 Header file 目錄 ====================
set (CMAKE_INCLUDE_CURRENT_DIR ON)
include_directories (
    ${CMAKE_CURRENT_SOURCE_DIR}
#    ${CUDA_INCLUDE_DIRS}
#    ${TENSORRT_INCLUDE_DIR}
#    ${OpenCV_INCLUDE_DIRS}
//------------------------------------
    ${PROJECTMAIN_PATH}
    # ${PROJECTMAIN_PATH}/callback
#    ${APPS_PATH}
#    ${GRFWIDGET_PATH}
#    ${MODELENGINE_PATH}
#    ${MODELENGINE_PATH}/conveyorMDEngine
#    ${MGRUTILS_PATH}
#    ${VIDEOCAPTURE_PATH}
#    ${SARIBBON_PATH}
#    ${IOTHANDLER_PATH}
)

#===========  Find all executables  ===================================
file  (GLOB PROJECT_SRCS
    ${PROJECTMAIN_SRC}
#    ${APPS_SRC}
#    ${GRFWIDGET_SRC}
#    ${MODELENGINE_SRC}
#    ${MGRUTILS_SRC}
#    ${VIDEOCAPTURE_SRC}
#    ${SARIBBON_SRC}
#    ${IOTHANDLER_SRC}
)
set ( CMAKE_BINARY_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set ( EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})

#==================================================================
add_executable (${executable_name} ${PROJECT_SRCS})

#======= Linker OpenCV, CUDA, TensorRT, yaml-cpp ==================================================
# target_link_libraries (${executable_name} Qt5::Widgets )
target_link_libraries (${executable_name} ${OpenCV_LIBRARIES})
# target_link_libraries (${executable_name} ${OpenCV_LIBRARIES} -lyaml-cpp)
#target_link_libraries (${executable_name} ${CUDA_LIBRARIES} ${TENSORRT_LIBRARY})
#---  Link ffmpeg  ----------------------------------------------------------------
# target_link_libraries  (${executable_name} PkgConfig::LIBAV)
#------- spinnaker ----------------------------------------------
target_link_libraries (${executable_name} ${Spinnaker_LIBS})
target_include_directories (${executable_name} PUBLIC ${PROJECT_SOURCE_DIR} ${Spinnaker_INCLUDE_DIR})
