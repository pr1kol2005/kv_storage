add_library(kv_storage INTERFACE)

target_include_directories(kv_storage
  INTERFACE ${SOURCE_DIR}
)

target_link_libraries(kv_storage
  INTERFACE absl::btree absl::flat_hash_map
)
