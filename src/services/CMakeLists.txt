add_library(rtu_services STATIC
    auth/IAuthService.h
    subscription/ISubscriptionService.h
    subscription/SubscriptionTier.h
    ServiceLocator.h
    ServiceLocator.cpp
)

set_target_properties(rtu_services PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(rtu_services PUBLIC ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(rtu_services PUBLIC Qt6::Core Qt6::Network)
