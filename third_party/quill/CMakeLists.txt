FetchContent_Declare(
  quill
  GIT_REPOSITORY  https://github.com/odygrd/quill.git
  GIT_TAG         v11.0.2
  GIT_SHALLOW     TRUE
  SYSTEM
  EXCLUDE_FROM_ALL
)

FetchContent_MakeAvailable(quill)
