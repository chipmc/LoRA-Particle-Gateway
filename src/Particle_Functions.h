/**
 * @file Particle_Functions.h
 * @author Chip McClelland (chip@seeinisghts.com)
 * @brief This file initilizes the Particle functions and variables needed for control from the console / API calls
 * @version 0.1
 * @date 2022-06-30
 * 
 */

#ifndef __PARTICLE_FUNCTIONS_H
#define __PARTICLE_FUNCTIONS_H

#include "Particle.h"

/**
 * This class is a singleton; you do not create one as a global, on the stack, or with new.
 * 
 * From global application setup you must call:
 * Particle_Functions::instance().setup();
 * 
 * From global application loop you must call:
 * Particle_Functions::instance().loop();
 */
class Particle_Functions {
public:
    /**
     * @brief Gets the singleton instance of this class, allocating it if necessary
     * 
     * Use Particle_Functions::instance() to instantiate the singleton.
     */
    static Particle_Functions &instance();

    /**
     * @brief Perform setup operations; call this from global application setup()
     * 
     * You typically use Particle_Functions::instance().setup();
     */
    void setup();

    /**
     * @brief Perform application loop operations; call this from global application loop()
     * 
     * You typically use Particle_Functions::instance().loop();
     */
    void loop();
    /**
     * @brief Sets the how often the device will report data in minutes.
     *
     * @details Extracts the integer from the string passed in, and sets the closing time of the facility
     * based on this input. Fails if the input is invalid.
     *
     * @param command A string indicating the number of minutes between reporting events.  Note, this function
     * sets an interim value for reporting frequency which takes effect once sent to a new node.
     *
     * @return 1 if able to successfully take action, 0 if invalid command
     */
    int setFrequency(String command);
    /**
     * @brief Disconnects from the Particle network completely
     * 
     * @return true 
     * @return false 
     */
    bool disconnectFromParticle();


protected:
    /**
     * @brief The constructor is protected because the class is a singleton
     * 
     * Use Particle_Functions::instance() to instantiate the singleton.
     */
    Particle_Functions();

    /**
     * @brief The destructor is protected because the class is a singleton and cannot be deleted
     */
    virtual ~Particle_Functions();

    /**
     * This class is a singleton and cannot be copied
     */
    Particle_Functions(const Particle_Functions&) = delete;

    /**
     * This class is a singleton and cannot be copied
     */
    Particle_Functions& operator=(const Particle_Functions&) = delete;

    /**
     * @brief Singleton instance of this class
     * 
     * The object pointer to this class is stored here. It's NULL at system boot.
     */
    static Particle_Functions *_instance;

};
#endif  /* __PARTICLE_FUNCTIONS_H */
