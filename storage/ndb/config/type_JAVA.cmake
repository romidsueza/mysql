SET (CLASS_DIR "target/classes")
SET (JAR_DIR ".")

FILE(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CLASS_DIR})
FILE(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${JAR_DIR})

# compile all .java files with javac to .class
ADD_CUSTOM_COMMAND(
COMMAND echo "CLASSPATH: ${CLASSPATH}"
OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${JAVA_FILES}.class
COMMAND echo "${JAVA_COMPILE} -d ${CMAKE_CURRENT_SOURCE_DIR}/${CLASS_DIR}  -classpath ${CLASSPATH} ${JAVA_SOURCE_DIRS}"
COMMAND ${JAVA_COMPILE}
ARGS -d ${CMAKE_CURRENT_SOURCE_DIR}/${CLASS_DIR}
ARGS -classpath "${CLASSPATH}"
${JAVA_SOURCE_DIRS}
)

# build .jar file from .class files
ADD_CUSTOM_COMMAND(
COMMAND echo "JAR: ${CLASS_DIR}"
OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${JAR_FILE}
DEPENDS
${CMAKE_CURRENT_BINARY_DIR}/${JAVA_FILES}.class
COMMAND ${CMAKE_COMMAND}
ARGS -E chdir ${CMAKE_CURRENT_BINARY_DIR}
${JAVA_ARCHIVE} cfv ${JAR_DIR}/${JAR_FILE} -C ${CLASS_DIR} . 
)

# the target
ADD_CUSTOM_TARGET(
${JAR_FILE}
ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${JAR_FILE})

