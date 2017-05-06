file( GLOB CSources *.c )
add_library( fjson ${CSources} )
install( TARGETS fjson DESTINATION lib )
