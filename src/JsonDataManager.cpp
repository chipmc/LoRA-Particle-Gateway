#include "JsonDataManager.h"
#include "Room_Occupancy.h"							// Aggregates node data to get net room occupancy for Occupancy Nodes
#include "PublishQueuePosixRK.h"
#include "LocalTimeRK.h"					        // https://rickkas7.github.io/LocalTimeRK/

// Singleton instantiation - from template
JsonDataManager *JsonDataManager::_instance;

// [static]
JsonDataManager &JsonDataManager::instance() {
    if (!_instance) {
        _instance = new JsonDataManager();
    }
    return *_instance;
}

JsonDataManager::JsonDataManager() {
}

JsonDataManager::~JsonDataManager() {
}

// ************************************************************************
// ******** JSON Object - Scoped to JsonDataManager Class        ***********
// ************************************************************************
// JSON for node data
JsonParserStatic<3072, 550> jp;						// Make this global - reduce possibility of fragmentation
// So, here is how we arrived at this number:
// 1.  We need to store 50 nodes - each node is 6 bytes (nodeNumber, uniqueID, sensorType, payload, pendingAlerts) - 300 bytes
// 2.  We have one token for each object - 50 tokens and two tokens for each key value pair (50 * 5 * 2) - 500 tokens for a total of 550 tokens

LocalTimeConvert conv2;						// For local time

bool JsonDataManager::setup() {

	// Here is where we load the JSON object from memory and parse
	jp.addString(nodeDatabase.get_nodeIDJson());				// Read in the JSON string from memory
	Log.info("The node string is %d bytes",nodeDatabase.get_nodeIDJson().length());
	JsonDataManager::printNodeData(false);						// Print the node data to the log

	if (jp.parse()) Log.info("Parsed Successfully");
	else {
		nodeDatabase.resetNodeIDs();
		Log.info("Parsing error");
	}
	
	return true;
}

void JsonDataManager::loop() {
												
}

/************************************************************************
 **                     Node Management Functions                      **
 ************************************************************************

NodeID JSON structure

{nodes:[
	{
		{node:(int)nodeNumber},
		{uID: (uint32_t)uniqueID},
		{type: (int)sensorType},
		{p: (int)compressedJoinPayload},
		{pend: (int)pendingAlerts},
		{cont: (int)pendingAlertContext},
		{lrep: (int)lastReport},						 // unix timestamp
		{jd1: ** type-specific value defined below **},
		{jd2: ** type-specific value defined below **}
,

	***** Payload Data 1 and Payload Data 2 definitions *****

	For types 10 - 19 (Occupancy Counters)

		{jd1: (int)occupancyNet}, 
		{jd2: (int)occupancyGross}

	},
	....]
}

*/

// These functions interact with data in the nodeID JSON

byte JsonDataManager::getType(int nodeNumber) {
	int type;

	const JsonParserGeneratorRK::jsmntok_t *nodesArrayContainer;			// Token for the outer array
	jp.getValueTokenByKey(jp.getOuterObject(), "nodes", nodesArrayContainer);
	const JsonParserGeneratorRK::jsmntok_t *nodeObjectContainer;			// Token for the objects in the array (I beleive)

	nodeObjectContainer = jp.getTokenByIndex(nodesArrayContainer, nodeNumber-1);
	if(nodeObjectContainer == NULL) {
		Log.info("From getType function Node number not found so returning %d",current.get_sensorType());
		return current.get_sensorType();									// Ran out of entries, go with what was reported by the node
	} 
	

	jp.getValueByKey(nodeObjectContainer, "type", type);
	Log.info("Returning sensor type %d in getType",type);
	return type;
}

bool JsonDataManager::setType(int nodeNumber, int newType) {
	if (nodeNumber == 0 || nodeNumber == 255) return false;
	int type;
	uint32_t uniqueID;

	const JsonParserGeneratorRK::jsmntok_t *nodesArrayContainer;			// Token for the outer array
	jp.getValueTokenByKey(jp.getOuterObject(), "nodes", nodesArrayContainer);
	const JsonParserGeneratorRK::jsmntok_t *nodeObjectContainer;			// Token for the objects in the array (I beleive)

	nodeObjectContainer = jp.getTokenByIndex(nodesArrayContainer, nodeNumber-1);
	if(nodeObjectContainer == NULL) { 
		Log.info("Ran out of entries in node database - setType object parsing");
		return false;								// Ran out of entries 
	}

	JsonModifier mod(jp);

	jp.getValueByKey(nodeObjectContainer, "uID", uniqueID);
	jp.getValueByKey(nodeObjectContainer, "type", type);

	Log.info("Changing sensor type from %d to %d", type, newType);
	mod.insertOrUpdateKeyValue(nodeObjectContainer, "type",(int)newType);
	mod.insertOrUpdateKeyValue(nodeObjectContainer, "p",(int)0);			// New type so we need to zero the values
	mod.insertOrUpdateKeyValue(nodeObjectContainer, "pend",(int)0);
	mod.insertOrUpdateKeyValue(nodeObjectContainer, "cont",(int)0);
	mod.insertOrUpdateKeyValue(nodeObjectContainer, "lrep",(int)0);
	mod.insertOrUpdateKeyValue(nodeObjectContainer, "jd1",(int)0);
	mod.insertOrUpdateKeyValue(nodeObjectContainer, "jd2",(int)0);

	return true;
}

bool JsonDataManager::getJoinPayload(uint8_t nodeNumber) {
	bool result;
	if (nodeNumber == 0 || nodeNumber == 255) return false;
	uint8_t sensorType = JsonDataManager::instance().getType(nodeNumber);
	int compressedJoinPayloadInt;
	uint8_t compressedJoinPayload;

	const JsonParserGeneratorRK::jsmntok_t *nodesArrayContainer;			// Token for the outer array
	jp.getValueTokenByKey(jp.getOuterObject(), "nodes", nodesArrayContainer);
	const JsonParserGeneratorRK::jsmntok_t *nodeObjectContainer;			// Token for the objects in the array (I beleive)

	nodeObjectContainer = jp.getTokenByIndex(nodesArrayContainer, nodeNumber-1);
	if(nodeObjectContainer == NULL) {
		Log.info("From getJoinPayload function Node number not found so returning false");
		return false;
	} 
	// Else we will load the current values from the node
	jp.getValueByKey(nodeObjectContainer, "p", compressedJoinPayloadInt);
	compressedJoinPayload = static_cast<uint8_t>(compressedJoinPayloadInt);

	result = JsonDataManager::instance().hydrateJoinPayload(sensorType, compressedJoinPayload);

	if(result) Log.info("Loaded payload values of %d, %d, %d, %d", current.get_payload1(), current.get_payload2(), current.get_payload3(), current.get_payload4());
	return result;
}

bool JsonDataManager::setJoinPayload(uint8_t nodeNumber) {
	bool result;
	if (nodeNumber == 0 || nodeNumber == 255) return false;
	uint8_t sensorType = JsonDataManager::instance().getType(nodeNumber);
	uint8_t payload1;
	uint8_t payload2;
	uint8_t payload3;
	uint8_t payload4;
	int compressedJoinPayloadInt;
	uint8_t compressedJoinPayload;

	const JsonParserGeneratorRK::jsmntok_t *nodesArrayContainer;			// Token for the outer array
	jp.getValueTokenByKey(jp.getOuterObject(), "nodes", nodesArrayContainer);
	const JsonParserGeneratorRK::jsmntok_t *nodeObjectContainer;			// Token for the objects in the array (I beleive)

	nodeObjectContainer = jp.getTokenByIndex(nodesArrayContainer, nodeNumber-1);
	if(nodeObjectContainer == NULL) { 
		Log.info("Ran out of entries in node database - setJoinPayload object parsing");
		return false;								// Ran out of entries 
	}							// Ran out of entries

	jp.getValueByKey(nodeObjectContainer, "p", compressedJoinPayloadInt);
	compressedJoinPayload = static_cast<uint8_t>(compressedJoinPayloadInt);			// set compressedJoinPayload as the compressed JSON payload variables

	result = JsonDataManager::instance().parseJoinPayloadValues(sensorType, compressedJoinPayload, payload1, payload2, payload3, payload4);
	if (!result) 
	Log.info("Changing payload values from %d, %d, %d, %d", payload1, payload2, payload3, payload4);

	compressedJoinPayload = JsonDataManager::instance().getCompressedJoinPayload(sensorType);	// set compressedJoinPayload as the compressed currentData payload variables
	result = JsonDataManager::instance().parseJoinPayloadValues(sensorType, compressedJoinPayload, payload1, payload2, payload3, payload4);
	Log.info("Changed payload values to %d, %d, %d, %d", payload1, payload2, payload3, payload4);

	JsonModifier mod(jp);
	mod.insertOrUpdateKeyValue(nodeObjectContainer, "p", (int)compressedJoinPayload);
	
	saveNodeDatabase(jp);						

	return result;
}

byte JsonDataManager::getAlertCode(int nodeNumber) {
	if (nodeNumber == 0 || nodeNumber == 255) return 255;					// Not a configured node

	// This function returns the pending alert code for the node - if there is one
	// If there is not one, it will check to see if the park is closed and return 6 if it is

	int pendingAlert;

	const JsonParserGeneratorRK::jsmntok_t *nodesArrayContainer;			// Token for the outer array
	jp.getValueTokenByKey(jp.getOuterObject(), "nodes", nodesArrayContainer);
	const JsonParserGeneratorRK::jsmntok_t *nodeObjectContainer;			// Token for the objects in the array (I beleive)

	nodeObjectContainer = jp.getTokenByIndex(nodesArrayContainer, nodeNumber-1);
	if(nodeObjectContainer == NULL) {
		Log.info("From getAlertCode function, Node number not found");
		return 255;															// Ran out of entries 
	} 

	jp.getValueByKey(nodeObjectContainer, "pend", pendingAlert);
	if (pendingAlert == 0) pendingAlert = (current.get_openHours() == 1) ?  0 : 6;	// Set an alert code if the node is not in open hours - this will reset the current data

	return pendingAlert;
}

bool JsonDataManager::setAlertCode(int nodeNumber, int newAlert) {
	if (nodeNumber == 0 || nodeNumber == 255) return false;
	int currentAlert;

	const JsonParserGeneratorRK::jsmntok_t *nodesArrayContainer;			// Token for the outer array
	jp.getValueTokenByKey(jp.getOuterObject(), "nodes", nodesArrayContainer);
	const JsonParserGeneratorRK::jsmntok_t *nodeObjectContainer;			// Token for the objects in the array

	nodeObjectContainer = jp.getTokenByIndex(nodesArrayContainer, nodeNumber-1);	// find the entry for the node of interest
	if(nodeObjectContainer == NULL) { 
		Log.info("Ran out of entries in node database - setAlertCode object parsing");
		return false;								// Ran out of entries 
	}							// Ran out of entries - node number entry not found triggers alert

	jp.getValueByKey(nodeObjectContainer, "pend", currentAlert);			// Now we have the oject for the specific node
	Log.info("Changing pending alert from %d to %d", currentAlert, newAlert);

	JsonModifier mod(jp);
	mod.insertOrUpdateKeyValue(nodeObjectContainer, "pend", (int)newAlert);
	
	saveNodeDatabase(jp);						

	return true;
}

uint16_t JsonDataManager::getAlertContext(int nodeNumber) {
	if (nodeNumber == 0 || nodeNumber == 255) return 255;					// Not a configured node

	// This function returns the pending alert context for the node - if there is one
	int pendingAlertContext;

	const JsonParserGeneratorRK::jsmntok_t *nodesArrayContainer;			// Token for the outer array
	jp.getValueTokenByKey(jp.getOuterObject(), "nodes", nodesArrayContainer);
	const JsonParserGeneratorRK::jsmntok_t *nodeObjectContainer;			// Token for the objects in the array (I beleive)

	nodeObjectContainer = jp.getTokenByIndex(nodesArrayContainer, nodeNumber-1);
	if(nodeObjectContainer == NULL) {
		Log.info("From getAlertContext function, Node number not found");
		return 255;															// Ran out of entries 
	} 

	jp.getValueByKey(nodeObjectContainer, "cont", pendingAlertContext);

	return pendingAlertContext;
}

bool JsonDataManager::setAlertContext(int nodeNumber, int newAlertContext) {
	if (nodeNumber == 0 || nodeNumber == 255) return false;
	int currentAlertContext;

	const JsonParserGeneratorRK::jsmntok_t *nodesArrayContainer;			// Token for the outer array
	jp.getValueTokenByKey(jp.getOuterObject(), "nodes", nodesArrayContainer);
	const JsonParserGeneratorRK::jsmntok_t *nodeObjectContainer;			// Token for the objects in the array

	nodeObjectContainer = jp.getTokenByIndex(nodesArrayContainer, nodeNumber-1);	// find the entry for the node of interest
	if(nodeObjectContainer == NULL) { 
		Log.info("Ran out of entries in node database - setAlertContext object parsing");
		return false;								// Ran out of entries 
	}							// Ran out of entries - node number entry not found triggers alert

	jp.getValueByKey(nodeObjectContainer, "cont", currentAlertContext);			// Now we have the oject for the specific node
	Log.info("Changing pending alert context from %d to %d", currentAlertContext, newAlertContext);

	JsonModifier mod(jp);
	mod.insertOrUpdateKeyValue(nodeObjectContainer, "cont", (int)newAlertContext);
	
	saveNodeDatabase(jp);						// This updates the JSON object but doe not commit to to persistent storage

	return true;
}

uint16_t JsonDataManager::setNodeToken(uint8_t nodeNumber) {
	if (nodeNumber == 0 || nodeNumber == 255) return 0;
	uint16_t token = sysStatus.get_tokenCore() * nodeNumber;	// This is the token for the node - it is a function of the core token and the day of the month
	Log.info("Setting token for node %d to %d", nodeNumber, token);
	return token;
}

bool JsonDataManager::setJsonData1(int nodeNumber, int sensorType, int newJsonData1) {
	if (nodeNumber == 0 || nodeNumber == 255) return false;
	if (sensorType > 29) return false; 					// Return false if node is not a valid sensor type
	int jsonData1;

	const JsonParserGeneratorRK::jsmntok_t *nodesArrayContainer;			// Token for the outer array
	jp.getValueTokenByKey(jp.getOuterObject(), "nodes", nodesArrayContainer);
	const JsonParserGeneratorRK::jsmntok_t *nodeObjectContainer;			// Token for the objects in the array (I beleive)

	nodeObjectContainer = jp.getTokenByIndex(nodesArrayContainer, nodeNumber-1);
	if(nodeObjectContainer == NULL) { 
		Log.info("Ran out of entries in node database - setJsonData1 object parsing");
		return false;								// Ran out of entries 
	}

	jp.getValueByKey(nodeObjectContainer, "jd1", jsonData1);

	Log.info("Updating jsonData1 value from %d to %d", jsonData1, newJsonData1);

	JsonModifier mod(jp);
	mod.insertOrUpdateKeyValue(nodeObjectContainer, "jd1", (int)newJsonData1);
	
	saveNodeDatabase(jp);						// This updates the JSON object but does not commit to to persistent storage

	return true;
}

bool JsonDataManager::setJsonData2(int nodeNumber, int sensorType, int newJsonData2) {
	if (nodeNumber == 0 || nodeNumber == 255) return false;
	if (sensorType > 29) return false; 					// Return false if node is not a valid sensor type

	int jsonData2;

	const JsonParserGeneratorRK::jsmntok_t *nodesArrayContainer;			// Token for the outer array
	jp.getValueTokenByKey(jp.getOuterObject(), "nodes", nodesArrayContainer);
	const JsonParserGeneratorRK::jsmntok_t *nodeObjectContainer;			// Token for the objects in the array (I beleive)

	nodeObjectContainer = jp.getTokenByIndex(nodesArrayContainer, nodeNumber-1);
	if(nodeObjectContainer == NULL) { 
		Log.info("Ran out of entries in node database - setJsonData2 object parsing");
		return false;								// Ran out of entries 
	}

	jp.getValueByKey(nodeObjectContainer, "jd2", jsonData2);

	Log.info("Updating jsonData2 value from %d to %d", jsonData2, newJsonData2);

	JsonModifier mod(jp);
	mod.insertOrUpdateKeyValue(nodeObjectContainer, "jd2", (int)newJsonData2);
	
	saveNodeDatabase(jp);						// This updates the JSON object but does not commit to to persistent storage

	return true;
}

bool JsonDataManager::setLastReport(int nodeNumber, int newLastReport) {
	if (nodeNumber == 0 || nodeNumber == 255) return false;					// return false if node not configured

	const JsonParserGeneratorRK::jsmntok_t *nodesArrayContainer;			// Token for the outer array
	jp.getValueTokenByKey(jp.getOuterObject(), "nodes", nodesArrayContainer);
	const JsonParserGeneratorRK::jsmntok_t *nodeObjectContainer;			// Token for the objects in the array (I beleive)

	nodeObjectContainer = jp.getTokenByIndex(nodesArrayContainer, nodeNumber-1);
	if(nodeObjectContainer == NULL) { 
		Log.info("Ran out of entries in node database - setLastReport object parsing");
		return false;								// Ran out of entries 
	}

	Log.info("LastReport value for node %d set to %d", nodeNumber, newLastReport);

	JsonModifier mod(jp);
	mod.insertOrUpdateKeyValue(nodeObjectContainer, "lrep", (int)newLastReport);
	
	saveNodeDatabase(jp);						// This updates the JSON object but does not commit to to persistent storage

	return true;
}


/**********************************************************************
 **           Occupancy Specific Node Management Functions           **
 **********************************************************************/

uint16_t JsonDataManager::getOccupancyNetBySpace(int space) {
	char message[256];
	int occupancyNet;
	int occupancyNetTotal = 0;
	uint32_t uniqueID;
	int sensorType;
	uint8_t payload1;
	uint8_t payload2;
	uint8_t payload3;
	uint8_t payload4;
	int compressedJoinPayload;
	byte multiEntranceFlag = 0;
	
	Log.info("Searching JSON database for nodes with space == %d - getOccupancyNetBySpace", space);
	const JsonParserGeneratorRK::jsmntok_t *nodesArrayContainer;			// Token for the outer array
	jp.getValueTokenByKey(jp.getOuterObject(), "nodes", nodesArrayContainer);
	const JsonParserGeneratorRK::jsmntok_t *nodeObjectContainer;			// Token for the objects in the array (I beleive)

	for (int i = 0; i < 100; i++) {												// Iterate through the array looking for a match
		nodeObjectContainer = jp.getTokenByIndex(nodesArrayContainer, i);
		if(nodeObjectContainer == NULL) {
			break;															// Ran out of entries, break
		} 
		jp.getValueByKey(nodeObjectContainer, "p", compressedJoinPayload);  // Get the compressedJoinPayload
		jp.getValueByKey(nodeObjectContainer, "uID", uniqueID);  			// Get the uniqueID
		jp.getValueByKey(nodeObjectContainer, "type", sensorType);  		// Get the sensorType
		JsonDataManager::instance().parseJoinPayloadValues(sensorType, compressedJoinPayload, payload1, payload2, payload3, payload4); // extract the values
		if (payload1 == space) {
			if(sensorType < 10 || sensorType > 19){		// ignore nodes that are not occupancy sensors and throw an alert	
				snprintf(message, sizeof(message), "Node in space %d has wrong sensorType = %d. uID: %lu", space + 1, sensorType, uniqueID);
				Log.info(message);
				if (Particle.connected()) PublishQueuePosix::instance().publish("Alert", message, PRIVATE);
				continue;
			}
			jp.getValueByKey(nodeObjectContainer, "jd1", occupancyNet);	// Node is in the passed-in space!
			occupancyNetTotal += occupancyNet;										// add the occupancyNet to the total for the space
			if(payload3 == 1){ // Flag this space as multiEntrance if a node is multi entrance
				multiEntranceFlag = 1;
			}
			if(multiEntranceFlag && payload3 == 0){ // Throw an alert if one of the nodes in this space is not multiEntrance (they should all be) 
				snprintf(message, sizeof(message), "Node in space %d is not set to multiEntrance. uID: %lu", space + 1, uniqueID);
				Log.info(message);
				if (Particle.connected()) PublishQueuePosix::instance().publish("Alert", message, PRIVATE);
			}
		}	
	}
	if(occupancyNetTotal < 0) {	// if the total net occupancy is less than 0, set all nodes in the space to 0
		snprintf(message, sizeof(message), "Space %d has a negative value. Resetting all node counts to 0.", space + 1);
		Log.info(message);
		if (Particle.connected()) PublishQueuePosix::instance().publish("Alert", message, PRIVATE);
		JsonDataManager::instance().resetSpace(space);
		return 0; // and return 0 for this report.
	}
	return occupancyNetTotal;
}

uint16_t JsonDataManager::getOccupancyGrossBySpace(int space) {
	int occupancyGross;
	int occupancyGrossTotal = 0;
	int sensorType;
	uint8_t payload1;
	uint8_t payload2;
	uint8_t payload3;
	uint8_t payload4;
	int compressedJoinPayload;
	
	Log.info("Searching array for nodes with space = %d - getOccupancyGrossBySpace", space);
	const JsonParserGeneratorRK::jsmntok_t *nodesArrayContainer;			// Token for the outer array
	jp.getValueTokenByKey(jp.getOuterObject(), "nodes", nodesArrayContainer);
	const JsonParserGeneratorRK::jsmntok_t *nodeObjectContainer;			// Token for the objects in the array (I beleive)

	for (int i = 0; i < 100; i++) {											// Iterate through the array looking for a match
		nodeObjectContainer = jp.getTokenByIndex(nodesArrayContainer, i);
		if(nodeObjectContainer == NULL) {
			break;															// Ran out of entries, break
		} 
		jp.getValueByKey(nodeObjectContainer, "p", compressedJoinPayload);  // Get the compressedJoinPayload
		jp.getValueByKey(nodeObjectContainer, "type", sensorType);  // Get the compressedJoinPayload
		if (sensorType >= 10 && sensorType <= 19) {	// Ignore nodes that are not occupancy nodes in this function
			JsonDataManager::instance().parseJoinPayloadValues(sensorType, compressedJoinPayload, payload1, payload2, payload3, payload4); // extract the values
			if (payload1 == space) {
				jp.getValueByKey(nodeObjectContainer, "jd2", occupancyGross);	// Node is in the passed-in space!
				occupancyGrossTotal += occupancyGross;							// add the occupancyGross to the total for the space
			}
		}
	}

	return occupancyGrossTotal;
}

bool JsonDataManager::resetOccupancyNetCounts(){
	int nodeNumber;
	int OccupancyNet;
	
	Log.info("Resetting occupancy net counts");
	const JsonParserGeneratorRK::jsmntok_t *nodesArrayContainer;			// Token for the outer array
	jp.getValueTokenByKey(jp.getOuterObject(), "nodes", nodesArrayContainer);
	const JsonParserGeneratorRK::jsmntok_t *nodeObjectContainer;			// Token for the objects in the array (I beleive)

	for (int i = 0; i < 100; i++) {											// Iterate through all the nodes
		nodeObjectContainer = jp.getTokenByIndex(nodesArrayContainer, i);
		if(nodeObjectContainer == NULL) {
			break;															// Ran out of entries
		} 
		jp.getValueByKey(nodeObjectContainer, "node", nodeNumber);  		// Get the nodeNumber
		jp.getValueByKey(nodeObjectContainer, "jd1", OccupancyNet);  		// Get the current occupancyNet
		 // Ignore nodes that are not occupancy nodes in this function

		if (OccupancyNet != 0) {
			Log.info("Resetting node %d occupancyNet from %d to 0", nodeNumber, OccupancyNet);
			JsonDataManager::instance().setOccupancyNetForNode(nodeNumber, 0);
		}
		else {
			Log.info("Node %d occupancyNet is already 0", nodeNumber);
		}
	}
	return true;
}

bool JsonDataManager::resetOccupancyCounts(){
	int nodeNumber;
	int sensorType;

	Log.info("Resetting all occupancy counts - resetOccupancyCounts");
	const JsonParserGeneratorRK::jsmntok_t *nodesArrayContainer;			// Token for the outer array
	jp.getValueTokenByKey(jp.getOuterObject(), "nodes", nodesArrayContainer);
	const JsonParserGeneratorRK::jsmntok_t *nodeObjectContainer;			// Token for the objects in the array (I beleive)

	for (int i = 0; i < 100; i++) {											// Iterate through the array looking for a match
		nodeObjectContainer = jp.getTokenByIndex(nodesArrayContainer, i);
		if(nodeObjectContainer == NULL) {
			break;															// Ran out of entries - no match found
		} 
		jp.getValueByKey(nodeObjectContainer, "node", nodeNumber);  		// Get the nodeNumber
		jp.getValueByKey(nodeObjectContainer, "type", sensorType);  		// Get the sensorType
		if (sensorType >= 10 && sensorType <= 19) {	// Ignore nodes that are not occupancy nodes in this function
			JsonDataManager::instance().resetCurrentDataForNode(nodeNumber);
		}
	}
	return true;
}

bool JsonDataManager::setOccupancyNetForNode(int nodeNumber, int newOccupancyNet){
	if (nodeNumber == 0 || nodeNumber == 255) return false;					// return false if node not configured

	int sensorType;
	uint32_t uniqueID;
	char message[256];
	bool result = 0;
	uint8_t payload1;
	uint8_t payload2;
	uint8_t payload3;
	uint8_t payload4;
	int compressedJoinPayload;

	const JsonParserGeneratorRK::jsmntok_t *nodesArrayContainer;			// Token for the outer array
	jp.getValueTokenByKey(jp.getOuterObject(), "nodes", nodesArrayContainer);
	const JsonParserGeneratorRK::jsmntok_t *nodeObjectContainer;			// Token for the objects in the array (I beleive)

	nodeObjectContainer = jp.getTokenByIndex(nodesArrayContainer, nodeNumber-1);
	if(nodeObjectContainer == NULL) { 
		Log.info("Ran out of entries in node database - setOccupancyNetForNode object parsing");
		return false;								// Ran out of entries 
	}

	jp.getValueByKey(nodeObjectContainer, "type", sensorType);  		// Get the sensorType
	jp.getValueByKey(nodeObjectContainer, "uID", uniqueID);  			// Get the uniqueID
	jp.getValueByKey(nodeObjectContainer, "p", compressedJoinPayload);  // Get the compressedJoinPayload
	JsonDataManager::instance().parseJoinPayloadValues(sensorType, compressedJoinPayload, payload1, payload2, payload3, payload4); // extract the values

	if (sensorType >= 10 && sensorType <= 19) {							// Ignore nodes that are not occupancy nodes in this function
		result = JsonDataManager::instance().setAlertCode(nodeNumber, 12);         			  	/*** Queue up an alert code with alert context ***/
		result = JsonDataManager::instance().setAlertContext(nodeNumber, newOccupancyNet);  	  /*** These will be set to current in the Data Acknowledgement message ***/
		if(result) {
			snprintf(message, sizeof(message), "Node %lu net count set to %d", uniqueID, newOccupancyNet);
			Log.info(message);
			if (Particle.connected()) PublishQueuePosix::instance().publish("Setting Occupancy", message, PRIVATE);
			JsonDataManager::instance().setJsonData1(nodeNumber, sensorType, newOccupancyNet);   /*** Set the nodeDatabase representation to 0 as well ***/
			// Update Ubidots preemptively with battery = -10. This is interpreted by UpdateGatewayNodesAndSpaces as "set the occupancyNet value only"
			snprintf(message, sizeof(message), "{\"nodeUniqueID\":\"%lu\",\"battery\":%d,\"space\":%d,\"spaceNet\":%d,\"spaceGross\":%d}",\
			uniqueID, -10, payload1 + 1, Room_Occupancy::instance().getRoomNet(payload1), Room_Occupancy::instance().getRoomGross(payload1));
			PublishQueuePosix::instance().publish("Ubidots-LoRA-Occupancy-v2", message, PRIVATE | WITH_ACK);
		}
		else {	// if we failed to set the alert for this node, throw an Alert to particle
			snprintf(message, sizeof(message), "Node not reset due to failure in setAlertCode or setAlertContext. uID: %lu", uniqueID);
			Log.info(message);
			if (Particle.connected()) PublishQueuePosix::instance().publish("Alert", message, PRIVATE);
		}
	}

	return true;
}


/**********************************************************************
 **                         Helper Functions                         **
 **********************************************************************/

void JsonDataManager::printNodeData(bool publish) {
	int nodeNumber;
	uint32_t uniqueID;
	int sensorType;
	uint8_t  payload1;
	uint8_t  payload2;
	uint8_t  payload3;
	uint8_t  payload4;
	int compressedJoinPayload;
	int pendingAlertCode;
	int pendingAlertContext;
	int jsonData1;
	int jsonData2;
	int lastReport;
	char data[622];  // max size

	const JsonParserGeneratorRK::jsmntok_t *nodesArrayContainer;			// Token for the outer array
	jp.getValueTokenByKey(jp.getOuterObject(), "nodes", nodesArrayContainer);
	const JsonParserGeneratorRK::jsmntok_t *nodeObjectContainer;			// Token for the objects in the array (I beleive)

	for (int i = 0; i < 100; i++) {												// Iterate through the array looking for a match
		nodeObjectContainer = jp.getTokenByIndex(nodesArrayContainer, i);
		if(nodeObjectContainer == NULL) {
			break;								// Ran out of entries 
			Log.info("Ran out of entries in node database - printNodeData");
		} 
		jp.getValueByKey(nodeObjectContainer, "uID", uniqueID);
		jp.getValueByKey(nodeObjectContainer, "node", nodeNumber);
		jp.getValueByKey(nodeObjectContainer, "type", sensorType);
		jp.getValueByKey(nodeObjectContainer, "p", compressedJoinPayload);
		jp.getValueByKey(nodeObjectContainer, "pend", pendingAlertCode);
		jp.getValueByKey(nodeObjectContainer, "cont", pendingAlertContext);
		jp.getValueByKey(nodeObjectContainer, "jd1", jsonData1);
		jp.getValueByKey(nodeObjectContainer, "jd2", jsonData2);
		jp.getValueByKey(nodeObjectContainer, "lrep", lastReport);

		conv2.withTime(static_cast<time_t>(lastReport)).convert();

		JsonDataManager::instance().parseJoinPayloadValues(sensorType, compressedJoinPayload, payload1, payload2, payload3, payload4);

		// Type differentiated console printing
		switch (sensorType) {
			case 1 ... 9: {    						// Counter
				snprintf(data, sizeof(data), "Node %d, uniqueID %lu, type %d, payload (%d/%d/%d/%d) with pending alert %d and alert context %d, lastReport %s", nodeNumber, uniqueID, sensorType, payload1, payload2, payload3, payload4, pendingAlertCode, pendingAlertContext, conv2.timeStr().c_str());
			} break;
			case 10 ... 19: {   					// Occupancy
				snprintf(data, sizeof(data), "Node %d, uniqueID %lu, type %d, net %d, gross %d, payload (%d/%d/%d/%d) with pending alert %d and alert context %d, lastReport %s", nodeNumber, uniqueID, sensorType, jsonData1, jsonData2, payload1, payload2, payload3, payload4, pendingAlertCode, pendingAlertContext, conv2.timeStr().c_str());
			} break;
			case 20 ... 29: {   					// Sensor
				snprintf(data, sizeof(data), "Node %d, uniqueID %lu, type %d, payload (%d/%d/%d/%d) with pending alert %d and alert context %d, lastReport %s", nodeNumber, uniqueID, sensorType, payload1, payload2, payload3, payload4, pendingAlertCode, pendingAlertContext, conv2.timeStr().c_str());
			} break;
			default: {          		
				Log.info("Unknown sensor type in printNodeData %d", sensorType);
				if (Particle.connected()) PublishQueuePosix::instance().publish("Alert", "Unknown sensor type in printNodeData", PRIVATE);
			} break;
    	}

		Log.info(data);
		if (Particle.connected() && publish) {
			PublishQueuePosix::instance().publish("nodeData", data, PRIVATE);
			delay(1000);
		}
	}
	// Log.info(nodeDatabase.get_nodeIDJson());  // See the raw JSON string
}

uint8_t JsonDataManager::findNodeNumber(int nodeNumber, uint32_t uniqueID) {
	int index=1;															// Variables to hold values for the function
	int nodeDeviceNumber;
	uint32_t nodeDeviceID;
	int sensorType = (int)current.get_sensorType();

	const JsonParserGeneratorRK::jsmntok_t *nodesArrayContainer;			// Token for the outer array
	jp.getValueTokenByKey(jp.getOuterObject(), "nodes", nodesArrayContainer);
	const JsonParserGeneratorRK::jsmntok_t *nodeObjectContainer;			// Token for the objects in the array (I beleive)

	for (int i = 0; i < 100; i++) {												// Iterate through the array looking for a match
		nodeObjectContainer = jp.getTokenByIndex(nodesArrayContainer, i);
		if(nodeObjectContainer == NULL) {
			Log.info("findNodeNumber ran out of entries at i = %d",i);
			break;															// Ran out of entries - no match found
		} 
		jp.getValueByKey(nodeObjectContainer, "uID", nodeDeviceID);			// Get the deviceID and compare
		if (nodeDeviceID == uniqueID) {
			jp.getValueByKey(nodeObjectContainer, "node", nodeDeviceNumber);		// A match!
			return nodeDeviceNumber;												// All is good - return node number for the deviceID passed to the function
		}
		index++;															// This will be the node number for the next node if no match is found
	}
	// If we got to here, the nodeID was not a match for any entry and a new nodeNumer will be assigned
	nodeNumber = index;
	JsonModifier mod(jp);

	Log.info("New node will be assigned node number %d, nodeID of %lu ", nodeNumber, uniqueID);

	// Create node database entry
	mod.startAppend(jp.getOuterArray());
		mod.startObject();
		mod.insertKeyValue("node", nodeNumber);
		mod.insertKeyValue("uID", uniqueID);
		mod.insertKeyValue("type", sensorType);			// This is the sensor type reported by the node			
		mod.insertKeyValue("p",(int)0);
		mod.insertKeyValue("pend",(int)0);
		mod.insertKeyValue("cont",(int)0);
		mod.insertKeyValue("lrep",(int)Time.now());
		mod.insertKeyValue("jd1",(int)0);	
		mod.insertKeyValue("jd2",(int)0);
		mod.finishObjectOrArray();
	mod.finish();

	bool result = saveNodeDatabase(jp);									// This should backup the nodeID database - now updated to persistent storage

	// Set the configuration settings from the join payload into the new database entry
	JsonDataManager::instance().setJoinPayload(nodeNumber);

	if (!result) {
		if (Particle.connected()) PublishQueuePosix::instance().publish("Alert", "set_nodeIDJson failed to add a node to the database!!", PRIVATE);
	}

	return index;
}

bool JsonDataManager::checkIfNodeConfigured(int nodeNumber, uint32_t uniqueID)  {	// node is 'configured' if a uniqueID for it exists and the payload is set
	if (nodeNumber == 0 || nodeNumber == 255) return false;

	const JsonParserGeneratorRK::jsmntok_t *nodesArrayContainer;			// Token for the outer array
	jp.getValueTokenByKey(jp.getOuterObject(), "nodes", nodesArrayContainer);
	const JsonParserGeneratorRK::jsmntok_t *nodeObjectContainer;			// Token for the objects in the array (I beleive)

	nodeObjectContainer = jp.getTokenByIndex(nodesArrayContainer, nodeNumber-1);
	if(nodeObjectContainer == NULL) { 
		Log.info("Ran out of entries in node database - nodeConfigured object parsing");
		return false;								// Ran out of entries 
	}							// Ran out of entries - no match found
	
	jp.getValueByKey(nodeObjectContainer, "uID", uniqueID);					// Get the uniqueID for the node number in question

	if (uniqueID == current.get_uniqueID()) return true;
	else {
		Log.info("Node number is found but uniqueID is not a match - nodeConfigured");  // See the raw JSON string
		return false;
	}
}

bool JsonDataManager::uniqueIDExistsInDatabase(uint32_t uniqueID)  {			// node is 'configured' if a uniqueID for it exists in the payload is set
	uint32_t nodeDeviceID;
	const JsonParserGeneratorRK::jsmntok_t *nodesArrayContainer;			// Token for the outer array
	jp.getValueTokenByKey(jp.getOuterObject(), "nodes", nodesArrayContainer);
	const JsonParserGeneratorRK::jsmntok_t *nodeObjectContainer;			// Token for the objects in the array (I beleive)

	for (int i = 0; i < 100; i++) {												// Iterate through the array looking for a match
		nodeObjectContainer = jp.getTokenByIndex(nodesArrayContainer, i);
		if(nodeObjectContainer == NULL) {
			Log.info("uniqueIDExistsInDatabase ran out of entries at i = %d",i);
			break;															// Ran out of entries - no match found
		} 
		jp.getValueByKey(nodeObjectContainer, "uID", nodeDeviceID);			// Get the deviceID and compare
		if (nodeDeviceID == uniqueID) {										// A match!		
			Log.info("uniqueIDExistsInDatabase returned true");
			return true;													// All is good - return node number for the deviceID passed to the function
		}
	}
	return false;
}

byte JsonDataManager::getNodeNumberForUniqueID(uint32_t uniqueID) {
	int nodeNumber;
	int index = 1;
	uint32_t nodeDeviceID;

	Log.info("searching array for node with unique id = %lu", uniqueID);

	const JsonParserGeneratorRK::jsmntok_t *nodesArrayContainer;			// Token for the outer array
	jp.getValueTokenByKey(jp.getOuterObject(), "nodes", nodesArrayContainer);
	const JsonParserGeneratorRK::jsmntok_t *nodeObjectContainer;			// Token for the objects in the array (I beleive)

	for (int i = 0; i < 100; i++) {												// Iterate through the array looking for a match
		nodeObjectContainer = jp.getTokenByIndex(nodesArrayContainer, i);
		if(nodeObjectContainer == NULL) {										
			Log.info("getNodeNumberForUniqueID ran out of entries at i = %d",i);
			break;															// Ran out of entries - no match found
		} 
		jp.getValueByKey(nodeObjectContainer, "uID", nodeDeviceID);			// Get the deviceID and compare
		Log.info("comparing id %lu to uniqueID %lu", nodeDeviceID, uniqueID);
		if (nodeDeviceID == uniqueID) {
			jp.getValueByKey(nodeObjectContainer, "node", nodeNumber);		// A match!
			return nodeNumber;												// All is good - return node number for the deviceID passed to the function
		}
		index++;															// This will be the node number for the next node if no match is found
	}
	Log.info("No match found for uniqueID %lu returning", uniqueID);
	return 255;	// Return 255 if no match found
}

bool JsonDataManager::resetInactiveSpaces(int secondsInactive){
	bool result = false;
	const int maxSpaces = 64;
    std::vector<std::vector<std::vector<int>>> spaceNodes(maxSpaces); // Vector of vectors to hold nodes for each space
    // Clear all elements from the spaceNodes vector
    for (auto& space : spaceNodes) {
        space.clear(); // Clear each inner vector
    }

	Log.info("Searching for inactive spaces to reset - resetInactiveSpaces");
	
    const JsonParserGeneratorRK::jsmntok_t *nodesArrayContainer;
    jp.getValueTokenByKey(jp.getOuterObject(), "nodes", nodesArrayContainer);

    // Group nodes by their "space" number
	int i = 0;
    while (true) {
		const JsonParserGeneratorRK::jsmntok_t *nodeObjectContainer = jp.getTokenByIndex(nodesArrayContainer, i);
        if (nodeObjectContainer == NULL) {
            Log.info("resetInactiveSpaces ran out of entries at i = %d", i);
            break;
        } 

		int lastReport;
		int sensorType;
		int nodeNumber;
		int uniqueID;
		uint8_t payload1;
		uint8_t payload2;
		uint8_t payload3;
		uint8_t payload4;
		int compressedJoinPayload;

        jp.getValueByKey(nodeObjectContainer, "p", compressedJoinPayload);  // Get the compressedJoinPayload
		jp.getValueByKey(nodeObjectContainer, "node", nodeNumber);  		// Get the nodeNumber
		jp.getValueByKey(nodeObjectContainer, "uID", uniqueID);  		// Get the nodeNumber
		jp.getValueByKey(nodeObjectContainer, "type", sensorType);  		// Get the sensorType
		jp.getValueByKey(nodeObjectContainer, "lrep", lastReport);  		// Get the lastReport
		if (sensorType > 0 && sensorType <= 9) {	// Ignore nodes that have a Counter sensorType in this function (they do not have a space)
			i++;
			continue;  
		}
		JsonDataManager::instance().parseJoinPayloadValues(sensorType, compressedJoinPayload, payload1, payload2, payload3, payload4); // extract the payload values (space is payload1)
		
		spaceNodes[payload1].push_back({lastReport, uniqueID}); // add the node to its respective space
		
		i++;
    }

	for (int space = 0; space < maxSpaces; space++) {
		// Check if the space has any nodes
		if (spaceNodes[space].empty()) {
			// If the space is empty, skip it
			continue;
		}
		bool allInactive = true;
        for (const auto& node : spaceNodes[space]) {
			time_t time = Time.now();
            Log.info("Node: %d, Space=%d, LastReport=%d, Now=%d, lengthSinceReport = %d", node[1], space + 1, node[0], (int)time, (int)time - node[0]);
			if(time - node[0] <= secondsInactive){
				allInactive = false;
			}
		}
		if(allInactive){
			char message[128];
			snprintf(message, sizeof(message), "Space %d has been inactive for >=1 hour. Resetting the space and its nodes.", space + 1);
			Log.info(message);
			if (Particle.connected()) PublishQueuePosix::instance().publish("Inactive Space", message, PRIVATE);
			result = resetSpace(space);
		}
    }

    return result;
}

bool JsonDataManager::resetSpace(int space){
	char message[256];
	byte updateNeeded = 0;
	int sensorType;
	int jsonData1;
	int nodeNumber;
	uint32_t uniqueID;
	uint8_t payload1;
	uint8_t payload2;
	uint8_t payload3;
	uint8_t payload4;
	int compressedJoinPayload;
	bool result = 0;

	const JsonParserGeneratorRK::jsmntok_t *nodesArrayContainer;			// Token for the outer array
	jp.getValueTokenByKey(jp.getOuterObject(), "nodes", nodesArrayContainer);
	const JsonParserGeneratorRK::jsmntok_t *nodeObjectContainer;			// Token for the objects in the array (I beleive)

	for (int i = 0; i < 100; i++) {											// Iterate through the array looking for a match
		nodeObjectContainer = jp.getTokenByIndex(nodesArrayContainer, i);
		if(nodeObjectContainer == NULL) {
			break;													// Ran out of entries, return false
		} 
		jp.getValueByKey(nodeObjectContainer, "p", compressedJoinPayload);  // Get the compressedJoinPayload
		jp.getValueByKey(nodeObjectContainer, "node", nodeNumber);  		// Get the nodeNumber
		jp.getValueByKey(nodeObjectContainer, "uID", uniqueID);  		// Get the nodeNumber
		jp.getValueByKey(nodeObjectContainer, "type", sensorType);  		// Get the sensorType
		jp.getValueByKey(nodeObjectContainer, "jd1", jsonData1);  		// Get the sensorType
		JsonDataManager::instance().parseJoinPayloadValues(sensorType, compressedJoinPayload, payload1, payload2, payload3, payload4); // extract the values
		if (payload1 == space) {
			// Reset the node in the space based on the node's sensorType
			switch (sensorType) {
				case 1 ... 9: {    						// Counter
					// Do nothing (devices with Counter sensorTypes do not have a "space" in their payload - see README)
				} break;
				case 10 ... 19: {   					// Occupancy
						snprintf(message, sizeof(message), "Resetting node %d - resetSpace", nodeNumber);
						Log.info(message);
						if (Particle.connected()) PublishQueuePosix::instance().publish("Space Reset", message, PRIVATE);
						result = JsonDataManager::instance().setAlertCode(nodeNumber, 12);         			  /*** Queue up an alert code with alert context ***/
						result = JsonDataManager::instance().setAlertContext(nodeNumber, 0);  	  			  /*** These will be set to current in the Data Acknowledgement message ***/
						result = JsonDataManager::instance().setJsonData1(nodeNumber, sensorType, 0);		
						result = JsonDataManager::instance().setLastReport(nodeNumber, Time.now()); 		// We just pretended that the node sent a data report with 0, so mark that as a report	
						if (!result) {
							snprintf(message, sizeof(message), "Could not reset node %d - resetSpace", nodeNumber);
							Log.info(message);
							if (Particle.connected()) PublishQueuePosix::instance().publish("Alert", message, PRIVATE);		
							updateNeeded = 0;
						}
						if (Room_Occupancy::instance().getRoomNet(payload1) != 0 && Room_Occupancy::instance().getRoomGross(payload1) != 0){
							Log.info("Setting updateNeeded flag to 1 for space %d with non-zero net / Gross", payload1 + 1);
							updateNeeded = 1;
						}
						else {
							Log.info("Space %d already zeroed", payload1 + 1);
						}
				} break;
				case 20 ... 29: {   					// Sensor
					// Reset Sensor nodes in the space here
				} break;
				default: {          
					snprintf(message, sizeof(message), "Unknown sensor type %d in resetSpace", sensorType);
					Log.info(message);
					if (Particle.connected()) PublishQueuePosix::instance().publish("Alert", message, PRIVATE);	
					return false;	
				} break;
			}
		}
	}
	if(updateNeeded == 1){
		// Update Ubidots preemptively with battery = -10. This is interpreted by UpdateGatewayNodesAndSpaces as "set the occupancyNet value only"
		snprintf(message, sizeof(message), "{\"nodeUniqueID\":\"%lu\",\"battery\":%d,\"space\":%d,\"spaceNet\":%d,\"spaceGross\":%d}",\
		uniqueID, -10, payload1 + 1, Room_Occupancy::instance().getRoomNet(payload1), Room_Occupancy::instance().getRoomGross(payload1));
		PublishQueuePosix::instance().publish("Ubidots-LoRA-Occupancy-v2", message, PRIVATE | WITH_ACK);
	}
	return true;
}

bool JsonDataManager::resetCurrentDataForNode(int nodeNumber){			// This function only resets the current data (jd1 and jd2) for the node, not the system data (alert codes etc)
	if (nodeNumber == 0 || nodeNumber == 255) return false;					// return false if node not configured

	int sensorType;
	uint32_t uniqueID;
	char message[256];
	bool result = 0;
	int payload1;
	int payload2;
	// uint8_t payload3;
	// uint8_t payload4;
	// int compressedJoinPayload;

	Log.info("Resetting current data for node %d - resetCurrentDataForNode", nodeNumber);

	const JsonParserGeneratorRK::jsmntok_t *nodesArrayContainer;			// Token for the outer array
	jp.getValueTokenByKey(jp.getOuterObject(), "nodes", nodesArrayContainer);
	const JsonParserGeneratorRK::jsmntok_t *nodeObjectContainer;			// Token for the objects in the array (I beleive)

	nodeObjectContainer = jp.getTokenByIndex(nodesArrayContainer, nodeNumber-1);
	if(nodeObjectContainer == NULL) { 
		Log.info("Ran out of entries in node database - resetCurrentDataForNode object parsing");
		return false;								// Ran out of entries 
	}

	jp.getValueByKey(nodeObjectContainer, "type", sensorType);  		// Get the sensorType
	jp.getValueByKey(nodeObjectContainer, "uID", uniqueID);  			// Get the uniqueID
	// jp.getValueByKey(nodeObjectContainer, "p", compressedJoinPayload);  // Get the compressedJoinPayload
	jp.getValueByKey(nodeObjectContainer, "jd1", payload1);  		// Get the jd1 (occupancyNet)
	jp.getValueByKey(nodeObjectContainer, "jd2", payload2);  		// Get the jd2 (occupancyGross)

	// JsonDataManager::instance().parseJoinPayloadValues(sensorType, compressedJoinPayload, payload1, payload2, payload3, payload4); // extract the values

	if (payload1 != 0 || payload2 != 0) {
		Log.info("Node %d has non-zero net or gross - resetting", nodeNumber);
		switch (sensorType) {
		case 1 ... 9: {    						// Counter
			// Reset Counter sensorType here
		} break;
		case 10 ... 19: {   					// Occupancy
			result = JsonDataManager::instance().setAlertCode(nodeNumber, 6);         /*** Queue up an alert code to reset current data ***/
			if(result){
				JsonDataManager::instance().setJsonData1(nodeNumber, sensorType, 0);   // Set the JsonData1 to 0 for the node (occupancyNet)
				JsonDataManager::instance().setJsonData2(nodeNumber, sensorType, 0);   // Set the JsonData2 to 0 for the node (occupancyGross)
				// Send with battery = -10. This is interpreted by UpdateGatewayNodesAndSpaces as "set the occupancyNet value only", which we set to 0
				snprintf(message, sizeof(message), "{\"nodeUniqueID\":\"%lu\",\"battery\":%d,\"space\":%d,\"spaceNet\":%d,\"spaceGross\":%d}",\
				uniqueID, -10, payload1 + 1, Room_Occupancy::instance().getRoomNet(payload1), Room_Occupancy::instance().getRoomGross(payload1));
				if (Particle.connected()) PublishQueuePosix::instance().publish("Ubidots-LoRA-Occupancy-v2", message, PRIVATE | WITH_ACK);
			} else {	// if we failed to set the alert for this node, throw an Alert to particle
				snprintf(message, sizeof(message), "Node not reset due to failure in setAlertCode. uID: %lu", uniqueID);
				Log.info(message);
				if (Particle.connected()) PublishQueuePosix::instance().publish("Alert", message, PRIVATE);
				return false;
			}					
		} break;
		case 20 ... 29: {   					// Sensor
			// Reset Sensor sensorType here
		} break;
		default: {  
			snprintf(message, sizeof(message), "Unknown sensor type in resetCurrentDataForNode %d", sensorType);
			Log.info(message);
			if (Particle.connected()) PublishQueuePosix::instance().publish("Alert", message, PRIVATE);		        		
			return false;
		} break;
		}
	}
	else {
		Log.info("Node %d has net and gross already zeroed", nodeNumber);	// Nothing to do
	}

	return true;
}

bool JsonDataManager::resetAllDataForNode(int nodeNumber){
	if (nodeNumber == 0 || nodeNumber == 255) return false;					// return false if node not configured

	int sensorType;
	uint32_t uniqueID;
	char message[256];
	bool result = 0;
	uint8_t payload1;
	uint8_t payload2;
	uint8_t payload3;
	uint8_t payload4;
	int compressedJoinPayload;

	Log.info("Resetting current and system data for node %d - resetAllDataForNode", nodeNumber);

	const JsonParserGeneratorRK::jsmntok_t *nodesArrayContainer;			// Token for the outer array
	jp.getValueTokenByKey(jp.getOuterObject(), "nodes", nodesArrayContainer);
	const JsonParserGeneratorRK::jsmntok_t *nodeObjectContainer;			// Token for the objects in the array (I beleive)

	nodeObjectContainer = jp.getTokenByIndex(nodesArrayContainer, nodeNumber-1);
	if(nodeObjectContainer == NULL) { 
		Log.info("Ran out of entries in node database - resetAllDataForNode object parsing");
		return false;								// Ran out of entries 
	}

	jp.getValueByKey(nodeObjectContainer, "type", sensorType);  		// Get the sensorType
	jp.getValueByKey(nodeObjectContainer, "uID", uniqueID);  			// Get the uniqueID
	jp.getValueByKey(nodeObjectContainer, "p", compressedJoinPayload);  // Get the compressedJoinPayload
	JsonDataManager::instance().parseJoinPayloadValues(sensorType, compressedJoinPayload, payload1, payload2, payload3, payload4); // extract the values

	switch (sensorType) {
		case 1 ... 9: {    						// Counter
			// Reset Counter sensorType here
		} break;
		case 10 ... 19: {   					// Occupancy
			result = JsonDataManager::instance().setAlertCode(nodeNumber, 5);         /*** Queue up an alert code to reset both system and current data on node ***/
			if(result){
				JsonDataManager::instance().setJsonData1(nodeNumber, sensorType, 0);   // Set the JsonData1 to 0 for the node (occupancyNet)
				JsonDataManager::instance().setJsonData2(nodeNumber, sensorType, 0);   // Set the JsonData1 to 0 for the node (occupancyGross)
				// Send with battery = -10. This is interpreted by UpdateGatewayNodesAndSpaces as "set the occupancyNet value only", which we set to 0
				snprintf(message, sizeof(message), "{\"nodeUniqueID\":\"%lu\",\"battery\":%d,\"space\":%d,\"spaceNet\":%d,\"spaceGross\":%d}",\
				uniqueID, -10, payload1 + 1, Room_Occupancy::instance().getRoomNet(payload1), Room_Occupancy::instance().getRoomGross(payload1));
				if (Particle.connected()) PublishQueuePosix::instance().publish("Ubidots-LoRA-Occupancy-v2", message, PRIVATE | WITH_ACK);
			} else {	// if we failed to set the alert for this node, throw an Alert to particle
				snprintf(message, sizeof(message), "Node not reset due to failure in setAlertCode. uID: %lu", uniqueID);
				Log.info(message);
				if (Particle.connected()) PublishQueuePosix::instance().publish("Alert", message, PRIVATE);
				return false;
			}					
		} break;
		case 20 ... 29: {   					// Sensor
			// Reset Sensor sensorType here
		} break;
		default: {          		
			Log.info("Unknown sensor type in resetAllDataForNode %d", sensorType);
			if (Particle.connected()) PublishQueuePosix::instance().publish("Alert", "Unknown sensor type in resetAllDataForNode", PRIVATE);
			return false;
		} break;
	}

	return true;
}

bool JsonDataManager::saveNodeDatabase(JsonParser &jp) {
    // The first token is the outer object - here we get the total size of the object
    JsonParserGeneratorRK::jsmntok_t *tok = jp.getTokens();
    
    // Allocate memory for tempBuf dynamically
    char *tempBuf = (char*)malloc(tok->end - tok->start + 1);

    // Check if memory allocation was successful
    if (tempBuf != nullptr) {
        // Copy the content to tempBuf
        memcpy(tempBuf, jp.getBuffer() + tok->start, tok->end - tok->start);
        
        // Null-terminate the string
        tempBuf[tok->end - tok->start] = '\0';

        // Return the dynamically allocated string and save it
		nodeDatabase.set_nodeIDJson(tempBuf);									// This should backup the nodeID database - now updated to persistent storage
		nodeDatabase.flush(false);

		free(tempBuf);															// Free the memory (good practice)
        return true;
    } else {
        return "Memory allocation failure!!";
		return false;
    }
}



/**********************************************************************
 **                         Data Compression                         **
 **********************************************************************/

uint8_t JsonDataManager::getCompressedJoinPayload(uint8_t sensorType) {
    uint8_t data[4] = {0};
    uint8_t bitSizes[4] = {0};

    switch (sensorType) {
        case 1 ... 9: {    						// Counter
            bitSizes[0] = 1; // 2-Way (1 bit)
        } break;
        case 10 ... 19: {   					// Occupancy
            bitSizes[0] = 6; // space (6 bits)
            bitSizes[1] = 1; // placement (1 bit)
            bitSizes[2] = 1; // multi (1 bit)
        } break;
        case 20 ... 29: {   					// Sensor
            bitSizes[0] = 6; // space (6 bits)
            bitSizes[1] = 1; // placement (1 bit)
        } break;
        default: {          		
            Log.info("Unknown sensor type in getCompressedJoinPayload %d", sensorType);
            if (Particle.connected()) PublishQueuePosix::instance().publish("Alert", "Unknown sensor type in getCompressedJoinPayload", PRIVATE);
            return 0;
        } break;
    }

	data[0] = current.get_payload1();
	data[1] = current.get_payload2();
	data[2] = current.get_payload3();
	data[3] = current.get_payload4();

    return compressData(data, bitSizes);
}

bool JsonDataManager::hydrateJoinPayload(uint8_t sensorType, uint8_t compressedJoinPayload) {
    uint8_t data[4] = {0};
    uint8_t bitSizes[4] = {0};

    switch (sensorType) {
        case 1 ... 9: {    						// Counter
            bitSizes[0] = 1; // 2-Way (1 bit)
        } break;
        case 10 ... 19: {   					// Occupancy
            bitSizes[0] = 6; // space (6 bits)
            bitSizes[1] = 1; // placement (1 bit)
            bitSizes[2] = 1; // multi (1 bit)
        } break;
        case 20 ... 29: {   					// Sensor
            bitSizes[0] = 6; // space (6 bits)
            bitSizes[1] = 1; // placement (1 bit)
        } break;
        default: {
            Log.info("Unknown sensor type in hydrateJoinPayload %d", sensorType);
            if (Particle.connected()) PublishQueuePosix::instance().publish("Alert", "Unknown sensor type in hydrateJoinPayload", PRIVATE);
            return false;
        } break;
    }

    decompressData(compressedJoinPayload, data, bitSizes);

    current.set_payload1(data[0]);
    current.set_payload2(data[1]);
    current.set_payload3(data[2]);
    current.set_payload4(data[3]);

    return true;
}

bool JsonDataManager::parseJoinPayloadValues(uint8_t sensorType, uint8_t compressedJoinPayload, uint8_t& payload1, uint8_t& payload2, uint8_t& payload3, uint8_t& payload4) {
    uint8_t data[4] = {0};
    uint8_t bitSizes[4] = {0};

    switch (sensorType) {
        case 1 ... 9: {    						// Counter
            bitSizes[0] = 1; // 2-Way (1 bit)
        } break;
        case 10 ... 19: {   					// Occupancy
            bitSizes[0] = 6; // space (6 bits)
            bitSizes[1] = 1; // placement (1 bit)
            bitSizes[2] = 1; // multi (1 bit)
        } break;
        case 20 ... 29: {   					// Sensor
            bitSizes[0] = 6; // space (6 bits)
            bitSizes[1] = 1; // placement (1 bit)
        } break;
        default: {
            Log.info("Unknown sensor type in parseJoinPayloadValues %d", sensorType);
            if (Particle.connected()) PublishQueuePosix::instance().publish("Alert", "Unknown sensor type in parseJoinPayloadValues", PRIVATE);
            return false;
        } break;
    }

    decompressData(compressedJoinPayload, data, bitSizes);

    payload1 = data[0];
    payload2 = data[1];
    payload3 = data[2];
    payload4 = data[3];

    return true;
}

uint8_t JsonDataManager::compressData(uint8_t data[], uint8_t bitSizes[]) {
    uint8_t compressedData = 0;
    uint8_t bitOffset = 0;
    for (uint8_t i = 0; i < 4; ++i) {
        data[i] = (data[i] < (1 << bitSizes[i])) ? data[i] : ((1 << bitSizes[i]) - 1);
        compressedData |= (data[i] * (1 << bitOffset));
        bitOffset += bitSizes[i];
    }
    return compressedData;
}

void JsonDataManager::decompressData(uint8_t compressedData, uint8_t data[], uint8_t bitSizes[]) {
    uint8_t bitOffset = 0;
    for (uint8_t i = 0; i < 4; ++i) {
        data[i] = (compressedData >> bitOffset) & ((1 << bitSizes[i]) - 1);
        bitOffset += bitSizes[i];
    }
}