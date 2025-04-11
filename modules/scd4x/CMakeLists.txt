if(CONFIG_SCD4X)
  zephyr_library()
  zephyr_library_sources(
    src/scd4x.c
    )
endif()
