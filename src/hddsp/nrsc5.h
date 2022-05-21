#ifndef NRSC5_H_
#define NRSC5_H_

/** \file nrsc5.h
 * External API for the nrsc5 library.
 */

/*! \mainpage Quick API Reference
 *
 * The library API is documented in nrsc5.h -- see the *Functions* section.
 *
 * Useful information about the protocol is provided in standards
 * documents available from https://www.nrscstandards.org/
 *
 * Note: this documentation was *not* written by the authors of **nrsc5**;
 * its accuracy cannot be guaranteed.
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Definitions.
 */
#define NRSC5_SCAN_BEGIN  87.9e6
#define NRSC5_SCAN_END   107.9e6
#define NRSC5_SCAN_SKIP    0.2e6

#define NRSC5_MIME_PRIMARY_IMAGE    0xBE4B7536
#define NRSC5_MIME_STATION_LOGO     0xD9C72536
#define NRSC5_MIME_NAVTEQ           0x2D42AC3E
#define NRSC5_MIME_HERE_TPEG        0x82F03DFC
#define NRSC5_MIME_HERE_IMAGE       0xB7F03DFC
#define NRSC5_MIME_HD_TMC           0xEECB55B6
#define NRSC5_MIME_HDC              0x4DC66C5A /**< High Defn Coding audio */
#define NRSC5_MIME_TEXT             0xBB492AAC
#define NRSC5_MIME_JPEG             0x1E653E9C
#define NRSC5_MIME_PNG              0x4F328CA0
#define NRSC5_MIME_TTN_TPEG_1       0xB39EBEB2
#define NRSC5_MIME_TTN_TPEG_2       0x4EB03469
#define NRSC5_MIME_TTN_TPEG_3       0x52103469
#define NRSC5_MIME_TTN_STM_TRAFFIC  0xFF8422D7
#define NRSC5_MIME_TTN_STM_WEATHER  0xEF042E96

enum
{
    NRSC5_MODE_FM,
    NRSC5_MODE_AM
};

/*
 * Data types.
 */
enum
{
    NRSC5_SIG_COMPONENT_AUDIO,
    NRSC5_SIG_COMPONENT_DATA
};

/**  Represent a channel component.
 *
 * An element of a linked list that accompanies a nrsc5_sig_service_t
 * describing a component of a SIG record. This provides further
 * information about the audio or data in the channel.
 */
struct nrsc5_sig_component_t
{
    struct nrsc5_sig_component_t *next;  /**< Pointer to next element or NULL */
    uint8_t type; /**< NRSC5_SIG_SERVICE_AUDIO or NRSC5_SIG_SERVICE_DATA */
    uint8_t id;   /**< Component identifier, 0, 1, 2,... */
    union
    {
        /*! Data service information
         */
        struct {
            uint16_t port;  /**< distinguishes packets for this service */
            /** e.g. NRSC5_SERVICE_DATA_TYPE_AUDIO_RELATED_DATA */
            uint16_t service_data_type;
            uint8_t type;   /**< 0 for stream,  1 for packet, 3 for LOT */
            uint32_t mime;  /**< content, e.g. NRSC5_MIME_STATION_LOGO */
        } data;
        /*! Audio service information
         */
        struct {
            uint8_t port;   /**< distinguishes packets for this service */
            uint8_t type;   /**< 0 for stream,  1 for packet, 3 for LOT */
            uint32_t mime;  /**< content, e.g. NRSC5_MIME_HDC */
        } audio;
    };
};
/**
 * Defines a typename for struct nrsc5_sig_component_t
 */
typedef struct nrsc5_sig_component_t nrsc5_sig_component_t;

enum
{
    NRSC5_SIG_SERVICE_AUDIO,
    NRSC5_SIG_SERVICE_DATA
};

/**  Represent Station Information Guide (SIG) records
 *
 * Element of a linked list that will accompany an NRSC5_EVENT_SIG type event.
 * Each cell describes a channel (audio or data), with a name, e.g. "MPS"
 * is used to indicate main program service, "SPS1" to indicate
 * supplemental program service 1.  Each service may include a linked
 * list of type nrsc5_sig_component_t describing its components, which
 * may include both audio and other data.
 */
struct nrsc5_sig_service_t
{
    struct nrsc5_sig_service_t *next; /**< Pointer to next element or NULL */
    uint8_t type;      /**< NRSC5_SIG_SERVICE_AUDIO or NRSC5_SIG_SERVICE_DATA */
    uint16_t number;   /**< Channel number: 1,2,3,4 */
    const char *name;  /**< Channel name, e.g. "MPS" or "SPS1" */
    nrsc5_sig_component_t *components; /**< Head of linked list of components */
};
/**
 * Defines a typename for struct nrsc5_sig_service_t
 */
typedef struct nrsc5_sig_service_t nrsc5_sig_service_t;

enum
{
    NRSC5_EVENT_LOST_DEVICE,
    NRSC5_EVENT_IQ,
    NRSC5_EVENT_SYNC,
    NRSC5_EVENT_LOST_SYNC,
    NRSC5_EVENT_MER,
    NRSC5_EVENT_BER,
    NRSC5_EVENT_HDC,
    NRSC5_EVENT_AUDIO,
    NRSC5_EVENT_ID3,
    NRSC5_EVENT_SIG,
    NRSC5_EVENT_LOT,
    NRSC5_EVENT_SIS
};

enum
{
    NRSC5_ACCESS_PUBLIC,
    NRSC5_ACCESS_RESTRICTED
};

enum
{
    NRSC5_PROGRAM_TYPE_UNDEFINED = 0,
    NRSC5_PROGRAM_TYPE_NEWS = 1,
    NRSC5_PROGRAM_TYPE_INFORMATION = 2,
    NRSC5_PROGRAM_TYPE_SPORTS = 3,
    NRSC5_PROGRAM_TYPE_TALK = 4,
    NRSC5_PROGRAM_TYPE_ROCK = 5,
    NRSC5_PROGRAM_TYPE_CLASSIC_ROCK = 6,
    NRSC5_PROGRAM_TYPE_ADULT_HITS = 7,
    NRSC5_PROGRAM_TYPE_SOFT_ROCK = 8,
    NRSC5_PROGRAM_TYPE_TOP_40 = 9,
    NRSC5_PROGRAM_TYPE_COUNTRY = 10,
    NRSC5_PROGRAM_TYPE_OLDIES = 11,
    NRSC5_PROGRAM_TYPE_SOFT = 12,
    NRSC5_PROGRAM_TYPE_NOSTALGIA = 13,
    NRSC5_PROGRAM_TYPE_JAZZ = 14,
    NRSC5_PROGRAM_TYPE_CLASSICAL = 15,
    NRSC5_PROGRAM_TYPE_RHYTHM_AND_BLUES = 16,
    NRSC5_PROGRAM_TYPE_SOFT_RHYTHM_AND_BLUES = 17,
    NRSC5_PROGRAM_TYPE_FOREIGN_LANGUAGE = 18,
    NRSC5_PROGRAM_TYPE_RELIGIOUS_MUSIC = 19,
    NRSC5_PROGRAM_TYPE_RELIGIOUS_TALK = 20,
    NRSC5_PROGRAM_TYPE_PERSONALITY = 21,
    NRSC5_PROGRAM_TYPE_PUBLIC = 22,
    NRSC5_PROGRAM_TYPE_COLLEGE = 23,
    NRSC5_PROGRAM_TYPE_SPANISH_TALK = 24,
    NRSC5_PROGRAM_TYPE_SPANISH_MUSIC = 25,
    NRSC5_PROGRAM_TYPE_HIP_HOP = 26,
    NRSC5_PROGRAM_TYPE_WEATHER = 29,
    NRSC5_PROGRAM_TYPE_EMERGENCY_TEST = 30,
    NRSC5_PROGRAM_TYPE_EMERGENCY = 31,
    NRSC5_PROGRAM_TYPE_TRAFFIC = 65,
    NRSC5_PROGRAM_TYPE_SPECIAL_READING_SERVICES = 76
};

/**
 * Station Information Service *Audio* service descriptor. This is a
 * linked list element that may point to further audio service
 * descriptor elements.  Refer to NRSC-5 document SY_IDD_1020s.
 */
struct nrsc5_sis_asd_t
{
    struct nrsc5_sis_asd_t *next; /**< Pointer to next element or NULL */
    unsigned int program;     /**< program number 0, 1, ..., 7 */
    unsigned int access;      /**< NRSC5_ACCESS_PUBLIC or NRSC5_ACCESS_RESTRICTED */
    unsigned int type;        /**< audio service, e.g. NRSC5_PROGRAM_TYPE_JAZZ  */
    unsigned int sound_exp;   /**< 0 is none, 2 is Dolby Pro Logic II Surround */
};
/**
 * Defines a typename for struct nrsc5_sis_asd_t
 */
typedef struct nrsc5_sis_asd_t nrsc5_sis_asd_t;

enum
{
    NRSC5_SERVICE_DATA_TYPE_NON_SPECIFIC = 0,
    NRSC5_SERVICE_DATA_TYPE_NEWS = 1,
    NRSC5_SERVICE_DATA_TYPE_SPORTS = 3,
    NRSC5_SERVICE_DATA_TYPE_WEATHER = 29,
    NRSC5_SERVICE_DATA_TYPE_EMERGENCY = 31,
    NRSC5_SERVICE_DATA_TYPE_TRAFFIC = 65,
    NRSC5_SERVICE_DATA_TYPE_IMAGE_MAPS = 66,
    NRSC5_SERVICE_DATA_TYPE_TEXT = 80,
    NRSC5_SERVICE_DATA_TYPE_ADVERTISING = 256,
    NRSC5_SERVICE_DATA_TYPE_FINANCIAL = 257,
    NRSC5_SERVICE_DATA_TYPE_STOCK_TICKER = 258,
    NRSC5_SERVICE_DATA_TYPE_NAVIGATION = 259,
    NRSC5_SERVICE_DATA_TYPE_ELECTRONIC_PROGRAM_GUIDE = 260,
    NRSC5_SERVICE_DATA_TYPE_AUDIO = 261,
    NRSC5_SERVICE_DATA_TYPE_PRIVATE_DATA_NETWORK = 262,
    NRSC5_SERVICE_DATA_TYPE_SERVICE_MAINTENANCE = 263,
    NRSC5_SERVICE_DATA_TYPE_HD_RADIO_SYSTEM_SERVICES = 264,
    NRSC5_SERVICE_DATA_TYPE_AUDIO_RELATED_DATA = 265
};

/**
 * Station Information Service *Data* service descriptor. This is a
 * linked list element that may point to further data service descriptors
 * via `next` member if not `NULL`.  See
 * nrsc5_service_data_type_name() for named types of data.
 * Refer to NRSC-5 document SY_IDD_1020s.
 */
struct nrsc5_sis_dsd_t
{
    struct nrsc5_sis_dsd_t *next; /**< Pointer to next element or NULL */
    unsigned int access;  /**< NRSC5_ACCESS_PUBLIC or NRSC5_ACCESS_RESTRICTED */
    unsigned int type; /**< data service type, e.g. NRSC5_SERVICE_DATA_TYPE_TEXT */
    uint32_t mime_type;   /**< MIME type, e.g. `NRSC5_MIME_TEXT` */
};
/**
 * Defines a typename for struct nrsc5_sis_dsd_t
 */
typedef struct nrsc5_sis_dsd_t nrsc5_sis_dsd_t;

/**  Incoming event from receiver.
 *
 * This event structure is passed to your application supplied
 * callback function--see nrsc5_set_callback().  It is a union of
 * various structs, keyed by member `event`.
 */
struct nrsc5_event_t
{
/*! Type of event.
 * The member `event` determines which sort of event occurred:
 * - `NRSC5_EVENT_LOST_DEVICE` : signal is over
 * - `NRSC5_EVENT_BER` : Bit Error Ratio data, see the `ber` union member
 * - `NRSC5_EVENT_MER` : modulation error ratio, see the `mer` union member,
 *   and NRSC5 document SY_TN_2646s
 * - `NRSC5_EVENT_IQ` : IQ data, see the `iq` union member
 * - `NRSC5_EVENT_HD`C : HDC audio packet, see the `hdc` union member
 * - `NRSC5_EVENT_AUDIO` : audio buffer, see the `audio` union member
 * - `NRSC5_EVENT_SYNC` : indicates synchronization achieved
 * - `NRSC5_EVENT_LOST_SYNC` : indicates synchronization lost
 * - `NRSC5_EVENT_ID3` : ID3 information packet arrived, see `id3` member
 *    and information in HD-Radio document SY_IDD_1028s.
 * - `NRSC5_EVENT_SIG` : service information arrived, see `sig` member
 * - `NRSC5_EVENT_LOT` : LOT file data available, see `lot` member
 * - `NRSC5_EVENT_SIS` : station information, see `sis` member
 */
    unsigned int event;
    union
    {
        struct {
            const void *data;
            size_t count;
        } iq;
        struct {
            float cber;
        } ber;
        struct {
            float lower;
            float upper;
        } mer;
        struct {
            unsigned int program;
            const uint8_t *data;
            size_t count;
        } hdc;
        struct {
            unsigned int program;
            const int16_t *data;
            size_t count;
        } audio;
        struct {
            unsigned int program;
            const char *title;
            const char *artist;
            const char *album;
            const char *genre;
			struct {
				unsigned int size;
				const uint8_t* data;
			} raw;
            struct {
                const char *owner;
                const char *id;
            } ufid;
            struct {
                uint32_t mime;
                int param;
                int lot;
            } xhdr;
        } id3;
        struct {
            uint16_t port;
            unsigned int lot;
            unsigned int size;
            uint32_t mime;
            const char *name;
            const uint8_t *data;
        } lot;
        struct {
            nrsc5_sig_service_t *services;
        } sig;
        struct {
            const char *country_code;
            int fcc_facility_id;
            const char *name;
            const char *slogan;
            const char *message;
            const char *alert;
            float latitude;
            float longitude;
            int altitude;
            nrsc5_sis_asd_t *audio_services;
            nrsc5_sis_dsd_t *data_services;
        } sis;
    };
};
/**
 * Defines a typename for struct nrsc5_event_t
 */
typedef struct nrsc5_event_t nrsc5_event_t;

/**
 * Prototype for a user event handling callback function.
 * The function will be invoked with a pointer to the nrsc5_event_t
 * and a pointer to a second contextual argument, established by
 * nrsc5_set_callback().
 */
typedef void (*nrsc5_callback_t)(const nrsc5_event_t *evt, void *opaque);

/**
 * An opaque data type used by API functions to represent session information.
 * Applications should acquire a pointer to one via the `open_` functions:
 * nrsc5_open(), nrsc5_open_file(), nrsc5_open_pipe(),  or nrsc5_open_rtltcp().
 */
typedef struct nrsc5_t nrsc5_t;


/* ============================================================================
 * Public functions. All functions return void or an error code (0 == success).
 */

/**
 * Retrieves a string corresponding to a service data type.
 * @param[in]  type  a service data type integer.
 * @param[out] name  character pointer to a string naming the service type
 * @return Nothing is returned
 *
 * This name will be quite short, e.g. "News" or "Weather". If the type is
 * not recognized, it will the string "Unknown".
 */
void nrsc5_service_data_type_name(unsigned int type, const char **name);

/**
 * Retrieves a string corresponding to a program type.
 * @param[in]  type  a protram data type integer.
 * @param[out] name  character pointer to a string naming the service type
 * @return Nothing is returned
 *
 * This name will be quite short, e.g. "News" or "Rock". If the type is
 * not recognized, it will the string "Unknown".
 */
void nrsc5_program_type_name(unsigned int type, const char **name);

/**
 * Initializes a session for use with a pipe.
 * @param[out] st  handle for an `nrsc5_t`
 * @return 0 on success, nonzero on error
 *
 */
int nrsc5_open_pipe(nrsc5_t **st );

/**
 * Closes an nrsc5 session.
 * @param[in] st  pointer to an `nrsc5_t`
 * @return Nothing is returned.
 *
 * Any worker thread is signalled to exit, files and sockets are closed,
 * and I/O buffers are freed.
 */
void nrsc5_close(nrsc5_t *);

/**
 * Set the session mode to AM or FM.
 * @param[in] st  pointer to an `nrsc5_t` session object
 * @param[in] mode  either `NRSC5_MODE_FM` or `NRSC5_MODE_AM`
 * @return 0 on success or nonzero on error.
 *
 */
int nrsc5_set_mode(nrsc5_t *, int mode);

/**
 * Establish a callback function.
 *
 * @param[in] st  pointer to an `nrsc5_t` session object
 * @param[in] callback  pointer to an event handling function of two arguments
 * @param[in] opaque    pointer to the function's intended 2nd argument
 * @return Nothing is returned.
 *
 */
void nrsc5_set_callback(nrsc5_t *st, nrsc5_callback_t callback, void *opaque);


/**
 * Push an IQ array of 8-bit unsigned samples into the demodulator.
 *
 * @param[in] st  pointer to an `nrsc5_t` session object
 * @param[in] samples  pointer to an array 8-bit unsigned samples
 * @param[in] length   the number of samples in the array
 * @return 0 on success, nonzero on error
 *
 */
int nrsc5_pipe_samples_cu8(nrsc5_t *st, uint8_t *samples, unsigned int length);


/**
 * Push an IQ input array of 16-bit signed samples into the demodulator.
 *
 * @param[in] st  pointer to an `nrsc5_t` session object
 * @param[in] samples  pointer to an array 16-bit signed samples
 * @param[in] length   the number of samples in the array
 * @return 0 on success, nonzero on error
 *
 */
int nrsc5_pipe_samples_cs16(nrsc5_t *st, int16_t *samples, unsigned int length);

#ifdef __cplusplus
}
#endif

#endif /* NRSC5_H_ */

