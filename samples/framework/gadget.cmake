  # Reads input javascript file
  file(READ ${OZZ_INPUT_JS} OZZ_SAMPLE_JS)

  # Insert in the input gadget file
  configure_file("${CMAKE_CURRENT_LIST_DIR}/gadget.xml.in" ${OZZ_OUTPUT_XML})
  