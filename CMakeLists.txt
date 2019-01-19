# Минимально необходимая для сборки версия cmake.
CMAKE_MINIMUM_REQUIRED (VERSION 3.13.2)

# Имя библиотеки.
project (BUTTONS)

# Производится проверка на то, что параметры оптимизации
# проекта (библиотеки) заданы.
# Они должны быть заданы в корневом проекте.
# Пример:
# SET (BUTTONS_OPTIMIZATION_FLAGS "-O0;-g3;")
if (NOT DEFINED ${PROJECT_NAME}_OPTIMIZATION_FLAGS)
    message (FATAL_ERROR
             "The value of the variable <<${PROJECT_NAME}_OPTIMIZATION_FLAGS>> is not specified!")
endif ()

# Производится проверка на то, что параметры аппаратной
# части для компилляции проекта (библиотеки) заданы.
# Они должны быть заданы в корневом проекте.
# Параметры должны быть обязательно разделены через <<;>>.
# Пример:
# SET (BUTTONS_COMPILER_FLAGS "${MICROCONTROLLER_HARDWARE_FLAGS};${CPP_COMPILER_FLAGS};")
if (NOT DEFINED ${PROJECT_NAME}_COMPILER_FLAGS)
    message (FATAL_ERROR
             "The value of the variable <<${PROJECT_NAME}_COMPILER_FLAGS>> is not specified!")
endif ()

# Путь до файла user_os.h с прослойками под конкретную операционную систему.
# Должен быть обязательно указан.
if (NOT DEFINED USER_OS_H_PATH)
    message (FATAL_ERROR
            "The value of the variable <<USER_OS_H_PATH>> is not specified!")
endif()

# Путь к заголовочным файлам операционной системы.
if (NOT DEFINED USER_OS_PATH)
    message (FATAL_ERROR
            "The value of the variable <<USER_OS_PATH>> is not specified!")
endif()

# Для сборки требуется:
include_directories(
    ${${PROJECT_NAME}_CFG_H_PATH} # 1. Путь до файла с разрешающим компиляцию данного файла объявлением.
    ${USER_OS_H_PATH} # 2. Путь до файла user_os с прослойками под конкретную операционную систему.
    ${USER_OS_PATH} # 3. Путь к заголовочным файлам операционной системы.
    ${CMAKE_CURRENT_SOURCE_DIR}/buttons_core/inc
)

# Компиляции полежат все <<.cpp>> файлы из директории src.
file (GLOB ${PROJECT_NAME}_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/buttons_core/src/*.cpp")

# Собираем библиотеку из .obj файлов, полученных из файлов, добавленных выше.
add_library (${PROJECT_NAME} ${${PROJECT_NAME}_SOURCES})

# Компиляция файлов происходит по флагам из корневого CMakeList.
target_compile_options (${PROJECT_NAME} PRIVATE
                        "${${PROJECT_NAME}_COMPILER_FLAGS}${${PROJECT_NAME}_OPTIMIZATION_FLAGS}")

# Показ флагов компиляции.
add_custom_command (TARGET ${PROJECT_NAME} POST_BUILD
                    COMMENT
                    "${PROJECT_NAME} flags: ${${PROJECT_NAME}_COMPILER_FLAGS}${${PROJECT_NAME}_OPTIMIZATION_FLAGS}")

# Вывод коммита.
add_custom_command (TARGET ${PROJECT_NAME} POST_BUILD
                    COMMENT "${PROJECT_NAME} commit:"
                    COMMAND git rev-parse HEAD)

# Вывод размера библиотеки.
add_custom_command (TARGET ${PROJECT_NAME} POST_BUILD
                    DEPENDS ${PROJECT_NAME}.a
                    COMMENT "${PROJECT_NAME} size information:"
                    COMMAND cd ${PROJECT_BINARY_DIR}
                    COMMAND ${ARM_SIZE} lib${PROJECT_NAME}.a -t)