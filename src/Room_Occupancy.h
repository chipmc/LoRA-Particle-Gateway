/**
 * @file Room_Occupancy.h
 * @author Chip McClelland (chip@seeinisghts.com)
 * @brief Here we take the couns from the door sensors and aggregate them to get room totals.
 * @version 0.1 - Specific for MAFC - needs to be generalized
 * @date 2024-01-12
 * 
 */

#ifndef __ROOM_OCCUPANCY_H
#define __ROOM_OCCUPANCY_H

#include "Particle.h"
#include "MyPersistentData.h"
#include "LoRA_Functions.h"

#define roomCounts Room_Occupancy::instance()

/**
 * This class is a singleton; you do not create one as a global, on the stack, or with new.
 * 
 * From global application setup you must call:
 * Room_Occupancy::instance().setup();
 * 
 * From global application loop you must call:
 * Room_Occupancy::instance().loop();
 */
class Room_Occupancy {
public:
    /**
     * @brief Gets the singleton instance of this class, allocating it if necessary
     * 
     * Use Room_Occupancy::instance() to instantiate the singleton.
     */
    static Room_Occupancy &instance();

    /**
     * @brief Perform setup operations; call this from global application setup()
     * 
     * You typically use Room_Occupancy::instance().setup();
     */
    void setup();

    /**
     * @brief Perform application loop operations; call this from global application loop()
     * 
     * You typically use Room_Occupancy::instance().loop();
     */
    void loop();

    /**
     * @brief Reset all the room counts - done each day at midnight
     * 
     * @return true if successful
     */
    bool resetAllCounts();

    /**
     * @brief Reset all the room net counts - done each day at break time
     * 
     * @return true if successful
     */
    bool resetNetCounts();

    /**
     * @brief Manually sets the net counts for a specified node - also updates the node's space on Ubidots with the new net value
     * 
     * @return true if successful
     */
    bool setOccupancyNetForNode(int nodeNumber, int newOccupancyNet);

    /**
     * @brief This function returns the current net room count
     */
    int getRoomNet(int space);

    /**
     * @brief This function returns the current gross room count
     */
    int getRoomGross(int space);

protected:
    /**
     * @brief The constructor is protected because the class is a singleton
     * 
     * Use Room_Occupancy::instance() to instantiate the singleton.
     */
    Room_Occupancy();

    /**
     * @brief The destructor is protected because the class is a singleton and cannot be deleted
     */
    virtual ~Room_Occupancy();

    /**
     * This class is a singleton and cannot be copied
     */
    Room_Occupancy(const Room_Occupancy&) = delete;

    /**
     * This class is a singleton and cannot be copied
     */
    Room_Occupancy& operator=(const Room_Occupancy&) = delete;

    /**
     * @brief Singleton instance of this class
     * 
     * The object pointer to this class is stored here. It's NULL at system boot.
     */
    static Room_Occupancy *_instance;

};
#endif  /* __ROOM_OCCUPANCY_H */
