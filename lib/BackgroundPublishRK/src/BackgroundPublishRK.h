#pragma once

// Repository: https://github.com/rickkas7/BackgroundPublishRK
// License: MIT

#include "Particle.h"

#include <protocol_defs.h>

/**
 * @brief Internal state of the publish thread
 */
typedef enum {
    BACKGROUND_PUBLISH_IDLE = 0,	//!< Not currently publishing
    BACKGROUND_PUBLISH_REQUESTED,	//!< Publish started
    BACKGROUND_PUBLISH_STOP,		//!< Thread stopped (need to start again to publish)
} publish_thread_state_t;

/**
 * @brief Optional callback function
 *
 * This can either be a regular C or C++ function, or can be a C++11 lambda
 *
 * @param succeeded The publish succeeded
 *
 * @param event_name The event name (that was passed to .publish)
 *
 * @param event_data The event data (that was passed to .publish)
 *
 * @param event_context The context (that was passed to .publish). This is a place where you can include an arbitrary
 * pointer to a C++ class or state structure if you don't want to use a C++ lambda.
 */
typedef std::function<void(bool succeeded,
    const char *event_name,
    const char *event_data,
    const void *event_context)> PublishCompletedCallback;

/**
 * @brief Background publish class. You typically instantiate one of these as a global variable.
 */
class BackgroundPublishRK
{
public:
    /**
     * @brief Gets the singleton instance of this class.
     * 
     * The constructor to this class is private; you cannot create an instance of this object
     * on the stack, as a global, or using new. Instead, you must use the static instance()
     * method to get the singleton instance of this class, as there can only be one per
     * application.
     */
    static BackgroundPublishRK &instance();

    /**
     * @brief Start the background publish thread. Required!
     *
     * You typically call this from setup() using:
     * 
     * ```
     * BackgroundPublishRK::instance().start();
     * ```
     */
    void start();

    /**
     * @brief Stop the background publish thread
     *
     * Normally you start it and never stop it, but this method is provided for special cases.
     */
    void stop();

    /**
     * @brief Publish method. Use this instead of Particle.publish().
     *
     * @param name Event name to publish (required)
     *
     * @param data Event data (optional). Must be a c-string (null-terminated) if non-NULL.
     * Maximum length varies by Device OS, currently 622 bytes. Note that the data must be
     * UTF-8; you cannot send arbitrary binary data!
     *
     * @param flags The publish flash. Default = PRIVATE. You will often use `PRIVATE | WITH_ACK`
     * but can also use `PRIVATE | NO_ACK`.
     *
     * @param cb The callback function to call when the publish completes. Optional. Pass NULL
     * or omit the parameter if you don't need a callback. It can be a C function or a C++11 lambda.
     *
     * @param context Optional parameter passed to the callback. You can store a C++ object
     * instance or a state structure pointer here.
     */
    bool publish(const char *name,
        const char *data = NULL,
        PublishFlags flags = PRIVATE,
        PublishCompletedCallback cb = NULL,
        const void *context = NULL);

    /**
     * @brief Used internally to mutex lock to safely access data structures from multiple threads
     *
     * You do not need to use this under normal circumstances as publish() handles this internally.
     */
    void lock() { os_mutex_lock(mutex); };

    /**
     * @brief Used internally to mutex lock to safely access data structures from multiple threads
     */
    void unlock() { os_mutex_unlock(mutex); };

private:
    /**
     * @brief Constructor - This class is a singleton and you cannot construct one.
     */
    BackgroundPublishRK();

    /**
     * @brief Destructor
     *
     * You normally instantiate this class as a global variable so it will never be deleted, but if
     * you do choose to delete one, the thread will be stopped.
     */
    virtual ~BackgroundPublishRK();

    /**
     * @brief This class is not copyable
     */
    BackgroundPublishRK(const BackgroundPublishRK&) = delete;

    /**
     * @brief This class is not copyable
     */
    BackgroundPublishRK& operator=(const BackgroundPublishRK&) = delete;


    Thread *thread = NULL;		//!< Thread object pointer. Allocated during start()
    void thread_f();			//!< Thread function, passed to the Thread object
    os_mutex_t mutex;	//!< Mutex to protect access to class members from multiple threads
    volatile publish_thread_state_t state = BACKGROUND_PUBLISH_IDLE; //!< Current state

    // arguments for Particle.publish
    char event_name[particle::protocol::MAX_EVENT_NAME_LENGTH+1];	//!< name passed to publish
    char event_data[particle::protocol::MAX_EVENT_DATA_LENGTH+1];	//!< event data passed to publish (may be empty string)
    PublishFlags event_flags; 	//!< event flags, typically PRIVATE, PRIVATE | WITH_ACK, or PRIVATE | NO_ACK.
    // callback when publish completes
    PublishCompletedCallback completed_cb = NULL; 	//!< Completion callback (optional)
    const void *event_context = NULL; 		//!< Context passed to completion (optional)

    static BackgroundPublishRK *_instance; //!< Singleton instance of this class
};
