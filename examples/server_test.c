#include "open62541.h"
#include <signal.h>
#include <stdlib.h>

#define UA_TUTORIAL_GOS_TEST_VALUE_NODE_ID "test-value"
#define UA_TUTORIAL_GOS_TEST_VALUE_NAME UA_TUTORIAL_GOS_TEST_VALUE_NODE_ID
#define UA_TUTORIAL_GOS_TEST_VALUE_DISPLAY_NAME "Test value"
#define UA_TUTORIAL_GOS_TEST_VALUE_TYPE UA_TYPES_DOUBLE

static UA_Double generate(void) { return (UA_Double)(rand()); }

static UA_Double defaultValue(void) { return (UA_Double)(0.0); }

static void generateValue(UA_Server *server) {
  UA_Double real = generate();
  UA_Variant value;
  UA_Variant_setScalar(&value, &real, &UA_TYPES[UA_TUTORIAL_GOS_TEST_VALUE_TYPE]);
  UA_NodeId currentNodeId = UA_NODEID_STRING(1, UA_TUTORIAL_GOS_TEST_VALUE_NODE_ID);
  UA_Server_writeValue(server, currentNodeId, value);
}

static void addGeneratedVariable(UA_Server *server) {
  UA_Double real = defaultValue();
  UA_VariableAttributes attr = UA_VariableAttributes_default;
  attr.displayName = UA_LOCALIZEDTEXT("en-US", UA_TUTORIAL_GOS_TEST_VALUE_DISPLAY_NAME);
  attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
  UA_Variant_setScalar(&attr.value, &real, &UA_TYPES[UA_TUTORIAL_GOS_TEST_VALUE_TYPE]);

  UA_NodeId currentNodeId = UA_NODEID_STRING(1, UA_TUTORIAL_GOS_TEST_VALUE_NODE_ID);
  UA_QualifiedName currentName = UA_QUALIFIEDNAME(1, UA_TUTORIAL_GOS_TEST_VALUE_NAME);
  UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
  UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
  UA_NodeId variableTypeNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE);
  UA_Server_addVariableNode(server, currentNodeId, parentNodeId, parentReferenceNodeId, currentName, variableTypeNodeId,
                            attr, NULL, NULL);

  generateValue(server);
}

/**
 * Variable Value Callback
 */

static void beforeGeneration(UA_Server *server, const UA_NodeId *sessionId, void *sessionContext,
                             const UA_NodeId *nodeid, void *nodeContext, const UA_NumericRange *range,
                             const UA_DataValue *data) {
  UA_Double real = generate();
  UA_Variant value;
  UA_Variant_setScalar(&value, &real, &UA_TYPES[UA_TUTORIAL_GOS_TEST_VALUE_TYPE]);
  UA_NodeId currentNodeId = UA_NODEID_STRING(1, UA_TUTORIAL_GOS_TEST_VALUE_NODE_ID);
  UA_Server_writeValue(server, currentNodeId, value);
}

static void afterGeneration(UA_Server *server, const UA_NodeId *sessionId, void *sessionContext,
                            const UA_NodeId *nodeId, void *nodeContext, const UA_NumericRange *range,
                            const UA_DataValue *data) {
  UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "The variable was updated");
}


static void addValueCallbackToGeneratedVariable(UA_Server *server) {
  UA_NodeId currentNodeId = UA_NODEID_STRING(1, UA_TUTORIAL_GOS_TEST_VALUE_NODE_ID);
  UA_ValueCallback callback;
  callback.onRead = beforeGeneration;
  callback.onWrite = afterGeneration;
  UA_Server_setVariableNode_valueCallback(server, currentNodeId, callback);
}

/**
 * Variable Data Sources
 */

static UA_StatusCode readGenerated(UA_Server *server, const UA_NodeId *sessionId, void *sessionContext,
                                   const UA_NodeId *nodeId, void *nodeContext, UA_Boolean sourceTimeStamp,
                                   const UA_NumericRange *range, UA_DataValue *dataValue) {
  UA_Double real = generate();
  UA_Variant_setScalarCopy(&dataValue->value, &real, &UA_TYPES[UA_TUTORIAL_GOS_TEST_VALUE_TYPE]);
  dataValue->hasValue = true;
  return UA_STATUSCODE_GOOD;
}

static UA_StatusCode writeGenerated(UA_Server *server, const UA_NodeId *sessionId, void *sessionContext,
                                    const UA_NodeId *nodeId, void *nodeContext, const UA_NumericRange *range,
                                    const UA_DataValue *data) {
  UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Changing the generated value is not implemented");
  return UA_STATUSCODE_BADINTERNALERROR;
}

static void addGeneratedDataSourceVariable(UA_Server *server) {
  UA_VariableAttributes attr = UA_VariableAttributes_default;
  attr.displayName = UA_LOCALIZEDTEXT("en-US", UA_TUTORIAL_GOS_TEST_VALUE_DISPLAY_NAME " - data source");
  attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

  UA_NodeId currentNodeId = UA_NODEID_STRING(1, UA_TUTORIAL_GOS_TEST_VALUE_NODE_ID "-datasource");
  UA_QualifiedName currentName = UA_QUALIFIEDNAME(1, UA_TUTORIAL_GOS_TEST_VALUE_NODE_ID "-datasource");
  UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
  UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
  UA_NodeId variableTypeNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE);

  UA_DataSource timeDataSource;
  timeDataSource.read = readGenerated;
  timeDataSource.write = writeGenerated;
  UA_Server_addDataSourceVariableNode(server, currentNodeId, parentNodeId, parentReferenceNodeId, currentName,
                                      variableTypeNodeId, attr, timeDataSource, NULL, NULL);
}

/** It follows the main server code, making use of the above definitions. */

UA_Boolean running = true;
static void stopHandler(int sign) {
  UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
  running = false;
}


int main(void) {
  srand(666);

  signal(SIGINT, stopHandler);
  signal(SIGTERM, stopHandler);

  UA_ServerConfig *config = UA_ServerConfig_new_default();
  UA_Server *server = UA_Server_new(config);

  addGeneratedVariable(server);
  addValueCallbackToGeneratedVariable(server);
  addGeneratedDataSourceVariable(server);

  UA_StatusCode retval = UA_Server_run(server, &running);
  UA_Server_delete(server);
  UA_ServerConfig_delete(config);
  return (int)retval;
}
