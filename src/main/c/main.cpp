#if defined(__linux__) && !defined(_GNU_SOURCE)
#define _BSD_SOURCE
#define _GNU_SOURCE
#endif

#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include <iostream>
#include <thread>

extern "C"
{
#include "aeron/concurrent/aeron_spsc_rb.h"
}

typedef struct srv_shm_header_stct
{
    int64_t version;
    int64_t spsc_srv_inbound_length;
    int64_t spsc_srv_outbound_length;
} srv_shm_header_t;

const int64_t srvReadyVersion = 1;
const int64_t srvDefaultInboundLength = 64 * 1024 + AERON_RB_TRAILER_LENGTH;
const int64_t srvDefaultOutboundLength = 64 * 1024 + AERON_RB_TRAILER_LENGTH;

typedef struct srv_shm_mapped_file_stct
{
    void *addr;
    size_t length;
} srv_shm_mapped_file_t;

inline size_t srv_shm_file_length(int64_t inbound_length, int64_t outbound_length)
{
    return AERON_ALIGN(sizeof(srv_shm_header_t), AERON_CACHE_LINE_LENGTH) +
        (size_t)inbound_length + (size_t)outbound_length;
}

inline size_t srvShmInboundOffset()
{
    return AERON_ALIGN(sizeof(srv_shm_header_t), AERON_CACHE_LINE_LENGTH);
}

inline size_t srvShmOutboundOffset(int64_t inbound_length)
{
    return AERON_ALIGN(sizeof(srv_shm_header_t), AERON_CACHE_LINE_LENGTH) + (size_t)inbound_length;
}

#define SRV_BLOCK_SIZE (4 * 1024)
#define SRV_ERR(fmt, ...) fprintf(stderr, fmt, __VA_ARGS__)

#if (__linux__)
std::string srvFilePath()
{
    return std::string("/dev/shm/srv.dat");
}
#else
std::string srvFilePath()
{
    return std::string(getenv("TMPDIR")) + std::string("/srv.dat");
}
#endif

inline int srv_mmap_shm(
    srv_shm_mapped_file_t *mapped_file, const char *path, size_t total_length)
{
    int result = -1;

    mapped_file->length = total_length;

    int fd = open(path, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    if (fd >= 0)
    {
        if (ftruncate(fd, (off_t)mapped_file->length) >= 0)
        {
            int flags = MAP_SHARED;

#ifdef __linux__
            flags = flags | MAP_POPULATE;
#endif
            mapped_file->addr = mmap(NULL, mapped_file->length, PROT_READ | PROT_WRITE, flags, fd, 0);
            close(fd);

            if (MAP_FAILED != mapped_file->addr)
            {
#if !defined(MAP_POPULATE)
                for (size_t i = 0; i < mapped_file->length; i += SRV_BLOCK_SIZE)
                {
                    volatile uint8_t *first_page_byte = (volatile uint8_t *)mapped_file->addr + i;
                    *first_page_byte = 0;
                }
#endif

                result = 0;
            }
            else
            {
                SRV_ERR("Failed to mmap file: %s\n", path);
            }
        }
        else
        {
            SRV_ERR("Failed to ftruncate file: %s\n", path);
        }
    }
    else
    {
        SRV_ERR("Failed to open file: %s\n", path);
    }

    return result;
}

inline int srv_create_shm(
    srv_shm_mapped_file_t *mapped_file, const std::string &path, int64_t inbound_length, int64_t outbound_length)
{
    size_t total_length = srv_shm_file_length(inbound_length, outbound_length);

    if (srv_mmap_shm(mapped_file, path.c_str(), total_length) < 0)
    {
        return -1;
    }

    auto header = static_cast<srv_shm_header_t *>(mapped_file->addr);

    header->spsc_srv_inbound_length = inbound_length;
    header->spsc_srv_outbound_length = outbound_length;
    AERON_PUT_ORDERED(header->version, srvReadyVersion);
    return 0;
}

inline void srvIdle(size_t workCount)
{
    if (workCount > 0)
    {
        return;
    }

    std::this_thread::yield();
}

struct srvState
{
    aeron_spsc_rb_t inboundRingBuffer;
    aeron_spsc_rb_t outboundRingBuffer;
    bool running;

    srvState() : running(true)
    {
    }
};

void srv_rb_handler(int32_t typeId, const void *buffer, size_t length, void *clientd)
{
    srvState *state = (srvState *)clientd;

    std::cout << "Message " << std::to_string(typeId) << std::endl;

    if (aeron_spsc_rb_write(&state->outboundRingBuffer, typeId, buffer, length) < 0)
    {
        SRV_ERR("%s\n", "could not write to outbound");
        state->running = false;
    }
}

int main()
{
    srv_shm_mapped_file_t mappedFile;
    srvState state;

    if (srv_create_shm(&mappedFile, srvFilePath(), srvDefaultInboundLength, srvDefaultOutboundLength) < 0)
    {
        exit(EXIT_FAILURE);
    }

    if (aeron_spsc_rb_init(
        &state.inboundRingBuffer,
        (uint8_t *)mappedFile.addr + srvShmInboundOffset(),
        srvDefaultInboundLength) < 0)
    {
        SRV_ERR("%s\n", "Failed to aeron_spsc_rb_init srv inbound");
    }

    if (aeron_spsc_rb_init(
        &state.outboundRingBuffer,
        (uint8_t *)mappedFile.addr + srvShmOutboundOffset(srvDefaultInboundLength),
        srvDefaultOutboundLength) < 0)
    {
        SRV_ERR("%s\n", "Failed to aeron_spsc_rb_init srv outbound");
    }

    while (state.running)
    {
        const size_t result = aeron_spsc_rb_read(
            &state.inboundRingBuffer,
            srv_rb_handler, &state, 1);

        srvIdle(result);
    }

    return 0;
}
