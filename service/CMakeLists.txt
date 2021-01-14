#set(GATE_SRC service_gate.h service_gate.cpp data_buffer.cpp data_buffer.h)

#add_library(service_gate SHARED ${GATE_SRC})
#set_property(TARGET service_gate  PROPERTY CXX_STANDARD 20)
#set_target_properties(service_gate PROPERTIES PREFIX "")
#
#set(HARBOR_SRC service_harbor.cpp service_harbor.h)
#add_library(service_harbor SHARED ${HARBOR_SRC})
#set_property(TARGET service_harbor  PROPERTY CXX_STANDARD 20)
#set_target_properties(service_harbor PROPERTIES PREFIX "")

# 设置库生成路径
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/cservice)

include_directories(.)
# 遍历文件列表生成对应库
SET(FILE_LIST gate harbor master dummy async clua)
foreach (file ${FILE_LIST})
    add_library(${file} SHARED "service_${file}.cpp")
    set_property(TARGET ${file}  PROPERTY CXX_STANDARD 20)
    set_target_properties(${file} PROPERTIES PREFIX "")
endforeach()