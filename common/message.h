#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdint.h>

#define MAX_MESSAGE_SIZE 1024
#define MAX_FILENAME_LENGTH 256
#define FILE_SEGMENT_SIZE 512

// Message types
#define CP_TEXT_MESSAGE 1
#define CP_FILE_TRANSFER_REQUEST 2
#define CP_FILE_SEGMENT 3
#define CP_FILE_SEGMENT_ACK 4
#define CP_FILE_TRANSFER_COMPLETE 5

typedef struct {
    uint8_t type;
    uint16_t length;
} CP_Header;

typedef struct {
    CP_Header header;
    char content[MAX_MESSAGE_SIZE];
} CP_TextMessage;

typedef struct {
    CP_Header header;
    char filename[MAX_FILENAME_LENGTH];
    uint64_t file_size;
} CP_FileTransferRequest;

typedef struct {
    CP_Header header;
    uint32_t file_id;
    uint32_t segment_number;
    uint16_t segment_size;
    char data[FILE_SEGMENT_SIZE];
} CP_FileSegment;

typedef struct {
    CP_Header header;
    uint32_t file_id;
    uint32_t segment_number;
} CP_FileSegmentAck;

typedef struct {
    CP_Header header;
    uint32_t file_id;
} CP_FileTransferComplete;

void encode_text_message(CP_TextMessage *message, const char *text);
void decode_text_message(CP_TextMessage *message, char *text);
void encode_file_transfer_request(CP_FileTransferRequest *request, const char *filename, uint64_t file_size);
void decode_file_transfer_request(CP_FileTransferRequest *request, char *filename, uint64_t *file_size);
void encode_file_segment(CP_FileSegment *segment, uint32_t file_id, uint32_t segment_number, const char *data, uint16_t segment_size);
void decode_file_segment(CP_FileSegment *segment, uint32_t *file_id, uint32_t *segment_number, char *data, uint16_t *segment_size);
void encode_file_segment_ack(CP_FileSegmentAck *ack, uint32_t file_id, uint32_t segment_number);
void decode_file_segment_ack(CP_FileSegmentAck *ack, uint32_t *file_id, uint32_t *segment_number);
void encode_file_transfer_complete(CP_FileTransferComplete *complete, uint32_t file_id);
void decode_file_transfer_complete(CP_FileTransferComplete *complete, uint32_t *file_id);

#endif // MESSAGE_H
