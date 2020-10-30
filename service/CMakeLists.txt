set(GATE_SRC service_gate.h service_gate.cpp data_buffer.cpp data_buffer.h)

add_library(service_gate SHARED ${GATE_SRC})
set_property(TARGET service_gate  PROPERTY CXX_STANDARD 20)
set_target_properties(service_gate PROPERTIES PREFIX "")

set(HARBOR_SRC service_harbor.cpp service_harbor.h)
add_library(service_harbor SHARED ${HARBOR_SRC})
set_property(TARGET service_harbor  PROPERTY CXX_STANDARD 20)
set_target_properties(service_harbor PROPERTIES PREFIX "")