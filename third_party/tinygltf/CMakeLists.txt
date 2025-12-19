FetchContent_Declare(
  tinygltf
  GIT_REPOSITORY  https://github.com/syoyo/tinygltf.git
  GIT_TAG         v2.9.7
  EXCLUDE_FROM_ALL
  SYSTEM
)
FetchContent_MakeAvailable(tinygltf)
