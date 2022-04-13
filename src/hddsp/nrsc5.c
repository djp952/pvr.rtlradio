#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "private.h"

#ifdef __MINGW32__
#define NRSC5_API __declspec(dllexport)
#else
#define NRSC5_API
#endif

static void nrsc5_init(nrsc5_t *st)
{
    st->closed = 0;
    st->mode = NRSC5_MODE_FM;
    st->callback = NULL;

    output_init(&st->output, st);
    input_init(&st->input, st, &st->output);
}


NRSC5_API void nrsc5_service_data_type_name(unsigned int type, const char **name)
{
    switch (type)
    {
    case NRSC5_SERVICE_DATA_TYPE_NON_SPECIFIC: *name = "Non-specific"; break;
    case NRSC5_SERVICE_DATA_TYPE_NEWS: *name = "News"; break;
    case NRSC5_SERVICE_DATA_TYPE_SPORTS: *name = "Sports"; break;
    case NRSC5_SERVICE_DATA_TYPE_WEATHER: *name = "Weather"; break;
    case NRSC5_SERVICE_DATA_TYPE_EMERGENCY: *name = "Emergency"; break;
    case NRSC5_SERVICE_DATA_TYPE_TRAFFIC: *name = "Traffic"; break;
    case NRSC5_SERVICE_DATA_TYPE_IMAGE_MAPS: *name = "Image Maps"; break;
    case NRSC5_SERVICE_DATA_TYPE_TEXT: *name = "Text"; break;
    case NRSC5_SERVICE_DATA_TYPE_ADVERTISING: *name = "Advertising"; break;
    case NRSC5_SERVICE_DATA_TYPE_FINANCIAL: *name = "Financial"; break;
    case NRSC5_SERVICE_DATA_TYPE_STOCK_TICKER: *name = "Stock Ticker"; break;
    case NRSC5_SERVICE_DATA_TYPE_NAVIGATION: *name = "Navigation"; break;
    case NRSC5_SERVICE_DATA_TYPE_ELECTRONIC_PROGRAM_GUIDE: *name = "Electronic Program Guide"; break;
    case NRSC5_SERVICE_DATA_TYPE_AUDIO: *name = "Audio"; break;
    case NRSC5_SERVICE_DATA_TYPE_PRIVATE_DATA_NETWORK: *name = "Private Data Network"; break;
    case NRSC5_SERVICE_DATA_TYPE_SERVICE_MAINTENANCE: *name = "Service Maintenance"; break;
    case NRSC5_SERVICE_DATA_TYPE_HD_RADIO_SYSTEM_SERVICES: *name = "HD Radio System Services"; break;
    case NRSC5_SERVICE_DATA_TYPE_AUDIO_RELATED_DATA: *name = "Audio-Related Objects"; break;
    default: *name = "Unknown"; break;
    }
}

NRSC5_API void nrsc5_program_type_name(unsigned int type, const char **name)
{
    switch (type)
    {
    case NRSC5_PROGRAM_TYPE_UNDEFINED: *name = "None"; break;
    case NRSC5_PROGRAM_TYPE_NEWS: *name = "News"; break;
    case NRSC5_PROGRAM_TYPE_INFORMATION: *name = "Information"; break;
    case NRSC5_PROGRAM_TYPE_SPORTS: *name = "Sports"; break;
    case NRSC5_PROGRAM_TYPE_TALK: *name = "Talk"; break;
    case NRSC5_PROGRAM_TYPE_ROCK: *name = "Rock"; break;
    case NRSC5_PROGRAM_TYPE_CLASSIC_ROCK: *name = "Classic Rock"; break;
    case NRSC5_PROGRAM_TYPE_ADULT_HITS: *name = "Adult Hits"; break;
    case NRSC5_PROGRAM_TYPE_SOFT_ROCK: *name = "Soft Rock"; break;
    case NRSC5_PROGRAM_TYPE_TOP_40: *name = "Top 40"; break;
    case NRSC5_PROGRAM_TYPE_COUNTRY: *name = "Country"; break;
    case NRSC5_PROGRAM_TYPE_OLDIES: *name = "Oldies"; break;
    case NRSC5_PROGRAM_TYPE_SOFT: *name = "Soft"; break;
    case NRSC5_PROGRAM_TYPE_NOSTALGIA: *name = "Nostalgia"; break;
    case NRSC5_PROGRAM_TYPE_JAZZ: *name = "Jazz"; break;
    case NRSC5_PROGRAM_TYPE_CLASSICAL: *name = "Classical"; break;
    case NRSC5_PROGRAM_TYPE_RHYTHM_AND_BLUES: *name = "Rhythm and Blues"; break;
    case NRSC5_PROGRAM_TYPE_SOFT_RHYTHM_AND_BLUES: *name = "Soft Rhythm and Blues"; break;
    case NRSC5_PROGRAM_TYPE_FOREIGN_LANGUAGE: *name = "Foreign Language"; break;
    case NRSC5_PROGRAM_TYPE_RELIGIOUS_MUSIC: *name = "Religious Music"; break;
    case NRSC5_PROGRAM_TYPE_RELIGIOUS_TALK: *name = "Religious Talk"; break;
    case NRSC5_PROGRAM_TYPE_PERSONALITY: *name = "Personality"; break;
    case NRSC5_PROGRAM_TYPE_PUBLIC: *name = "Public"; break;
    case NRSC5_PROGRAM_TYPE_COLLEGE: *name = "College"; break;
    case NRSC5_PROGRAM_TYPE_SPANISH_TALK: *name = "Spanish Talk"; break;
    case NRSC5_PROGRAM_TYPE_SPANISH_MUSIC: *name = "Spanish Music"; break;
    case NRSC5_PROGRAM_TYPE_HIP_HOP: *name = "Hip-Hop"; break;
    case NRSC5_PROGRAM_TYPE_WEATHER: *name = "Weather"; break;
    case NRSC5_PROGRAM_TYPE_EMERGENCY_TEST: *name = "Emergency Test"; break;
    case NRSC5_PROGRAM_TYPE_EMERGENCY: *name = "Emergency"; break;
    case NRSC5_PROGRAM_TYPE_TRAFFIC: *name = "Traffic"; break;
    case NRSC5_PROGRAM_TYPE_SPECIAL_READING_SERVICES: *name = "Special Reading Services"; break;
    default: *name = "Unknown"; break;
    }
}

static nrsc5_t *nrsc5_alloc()
{
    nrsc5_t *st = calloc(1, sizeof(*st));
    return st;
}

NRSC5_API int nrsc5_open_pipe(nrsc5_t **result)
{
    nrsc5_t *st = nrsc5_alloc();
    nrsc5_init(st);

    *result = st;
    return 0;
}

NRSC5_API void nrsc5_close(nrsc5_t *st)
{
    if (!st)
        return;

    st->closed = 1;

    input_free(&st->input);
    output_free(&st->output);
    free(st);
}

NRSC5_API int nrsc5_set_mode(nrsc5_t *st, int mode)
{
    if (mode == NRSC5_MODE_FM || mode == NRSC5_MODE_AM)
    {
        st->mode = mode;
        input_set_mode(&st->input);
        return 0;
    }
    return 1;
}

NRSC5_API void nrsc5_set_callback(nrsc5_t *st, nrsc5_callback_t callback, void *opaque)
{
    st->callback = callback;
    st->callback_opaque = opaque;
}

NRSC5_API int nrsc5_pipe_samples_cu8(nrsc5_t *st, uint8_t *samples, unsigned int length)
{
    input_push_cu8(&st->input, samples, length);

    return 0;
}

NRSC5_API int nrsc5_pipe_samples_cs16(nrsc5_t *st, int16_t *samples, unsigned int length)
{
    input_push_cs16(&st->input, samples, length);

    return 0;
}

void nrsc5_report(nrsc5_t *st, const nrsc5_event_t *evt)
{
    if (st->callback)
        st->callback(evt, st->callback_opaque);
}

void nrsc5_report_lost_device(nrsc5_t *st)
{
    nrsc5_event_t evt;

    evt.event = NRSC5_EVENT_LOST_DEVICE;
    nrsc5_report(st, &evt);
}

void nrsc5_report_iq(nrsc5_t *st, const void *data, size_t count)
{
    nrsc5_event_t evt;

    evt.event = NRSC5_EVENT_IQ;
    evt.iq.data = data;
    evt.iq.count = count;
    nrsc5_report(st, &evt);
}

void nrsc5_report_sync(nrsc5_t *st)
{
    nrsc5_event_t evt;

    evt.event = NRSC5_EVENT_SYNC;
    nrsc5_report(st, &evt);
}

void nrsc5_report_lost_sync(nrsc5_t *st)
{
    nrsc5_event_t evt;

    evt.event = NRSC5_EVENT_LOST_SYNC;
    nrsc5_report(st, &evt);
}

void nrsc5_report_hdc(nrsc5_t *st, unsigned int program, const uint8_t *data, size_t count)
{
    nrsc5_event_t evt;

    evt.event = NRSC5_EVENT_HDC;
    evt.hdc.program = program;
    evt.hdc.data = data;
    evt.hdc.count = count;
    nrsc5_report(st, &evt);
}

void nrsc5_report_audio(nrsc5_t *st, unsigned int program, const int16_t *data, size_t count)
{
    nrsc5_event_t evt;

    evt.event = NRSC5_EVENT_AUDIO;
    evt.audio.program = program;
    evt.audio.data = data;
    evt.audio.count = count;
    nrsc5_report(st, &evt);
}

void nrsc5_report_mer(nrsc5_t *st, float lower, float upper)
{
    nrsc5_event_t evt;

    evt.event = NRSC5_EVENT_MER;
    evt.mer.lower = lower;
    evt.mer.upper = upper;
    nrsc5_report(st, &evt);
}

void nrsc5_report_ber(nrsc5_t *st, float cber)
{
    nrsc5_event_t evt;

    evt.event = NRSC5_EVENT_BER;
    evt.ber.cber = cber;
    nrsc5_report(st, &evt);
}

void nrsc5_report_lot(nrsc5_t *st, uint16_t port, unsigned int lot, unsigned int size, uint32_t mime, const char *name, const uint8_t *data)
{
    nrsc5_event_t evt;

    evt.event = NRSC5_EVENT_LOT;
    evt.lot.port = port;
    evt.lot.lot = lot;
    evt.lot.size = size;
    evt.lot.mime = mime;
    evt.lot.name = name;
    evt.lot.data = data;
    nrsc5_report(st, &evt);
}

static uint8_t convert_sig_component_type(uint8_t type)
{
    switch (type)
    {
    case SIG_COMPONENT_AUDIO: return NRSC5_SIG_COMPONENT_AUDIO;
    case SIG_COMPONENT_DATA: return NRSC5_SIG_COMPONENT_DATA;
    default:
        assert(0 && "Invalid component type");
        return 0;
    }
}

static uint8_t convert_sig_service_type(uint8_t type)
{
    switch (type)
    {
    case SIG_SERVICE_AUDIO: return NRSC5_SIG_SERVICE_AUDIO;
    case SIG_SERVICE_DATA: return NRSC5_SIG_SERVICE_DATA;
    default:
        assert(0 && "Invalid service type");
        return 0;
    }
}

void nrsc5_report_sig(nrsc5_t *st, sig_service_t *services, unsigned int count)
{
    nrsc5_sig_service_t *service = NULL;
    nrsc5_event_t evt;

    evt.event = NRSC5_EVENT_SIG;

    // convert internal structures to public structures
    for (unsigned int i = 0; i < count; i++)
    {
        nrsc5_sig_component_t *component = NULL;
        nrsc5_sig_service_t *prev = service;

        service = calloc(1, sizeof(nrsc5_sig_service_t));
        service->type = convert_sig_service_type(services[i].type);
        service->number = services[i].number;
        service->name = services[i].name;

        if (prev == NULL)
            evt.sig.services = service;
        else
            prev->next = service;

        for (unsigned int j = 0; j < MAX_SIG_COMPONENTS; j++)
        {
            nrsc5_sig_component_t *prevc = component;
            sig_component_t *internal = &services[i].component[j];

            if (internal->type == SIG_COMPONENT_NONE)
                continue;

            component = calloc(1, sizeof(nrsc5_sig_component_t));
            component->type = convert_sig_component_type(internal->type);
            component->id = internal->id;

            if (internal->type == SIG_COMPONENT_AUDIO)
            {
                component->audio.port = internal->audio.port;
                component->audio.type = internal->audio.type;
                component->audio.mime = internal->audio.mime;
            }
            else if (internal->type == SIG_COMPONENT_DATA)
            {
                component->data.port = internal->data.port;
                component->data.service_data_type = internal->data.service_data_type;
                component->data.type = internal->data.type;
                component->data.mime = internal->data.mime;
            }

            if (prevc == NULL)
                service->components = component;
            else
                prevc->next = component;
        }
    }

    nrsc5_report(st, &evt);

    // free the data structures
    for (service = evt.sig.services; service != NULL; )
    {
        void *p;
        nrsc5_sig_component_t *component;

        for (component = service->components; component != NULL; )
        {
            p = component;
            component = component->next;
            free(p);
        }

        p = service;
        service = service->next;
        free(p);
    }
}

void nrsc5_report_sis(nrsc5_t *st, const char *country_code, int fcc_facility_id, const char *name,
                      const char *slogan, const char *message, const char *alert,
                      float latitude, float longitude, int altitude, nrsc5_sis_asd_t *audio_services,
                      nrsc5_sis_dsd_t *data_services)
{
    nrsc5_event_t evt;

    evt.event = NRSC5_EVENT_SIS;
    evt.sis.country_code = country_code;
    evt.sis.fcc_facility_id = fcc_facility_id;
    evt.sis.name = name;
    evt.sis.slogan = slogan;
    evt.sis.message = message;
    evt.sis.alert = alert;
    evt.sis.latitude = latitude;
    evt.sis.longitude = longitude;
    evt.sis.altitude = altitude;
    evt.sis.audio_services = audio_services;
    evt.sis.data_services = data_services;

    nrsc5_report(st, &evt);
}
