#include "message.h"
#include <stdint.h> 
#include <string.h>

// Implement the encoding/decoding functions

void encode_text_message(CP_TextMessage *message, const char *content) {
    message->header.type = CP_TEXT_MESSAGE;
    message->header.length = strlen(content);
    strncpy(message->content, content, MAX_MESSAGE_SIZE);
}

void decode_text_message(CP_TextMessage *message, char *decoded_content) {
    strncpy(decoded_content, message->content, message->header.length);
    decoded_content[message->header.length] = '\0';
}

void encode_file_transfer_request(CP_FileTransferRequest *request, const char *filename, uint64_t file_size) {
    request->header.type = CP_FILE_TRANSFER_REQUEST;
    request->header.length = sizeof(CP_FileTransferRequest) - sizeof(CP_Header);
    strncpy(request->filename, filename, MAX_FILENAME_LENGTH);
    request->file_size = file_size;
}

void decode_file_transfer_request(CP_FileTransferRequest *request, char *filename, uint64_t *file_size) {
    strncpy(filename, request->filename, MAX_FILENAME_LENGTH);
    *file_size = request->file_size;
}

void encode_file_segment(CP_FileSegment *segment, uint32_t file_id, uint32_t segment_number, const char *data, uint16_t segment_size) {
    segment->header.type = CP_FILE_SEGMENT;
    segment->header.length = sizeof(CP_FileSegment) - sizeof(CP_Header) - FILE_SEGMENT_SIZE + segment_size;
    segment->file_id = file_id;
    segment->segment_number = segment_number;
    segment->segment_size = segment_size;
    memcpy(segment->data, data, segment_size);
}

void decode_file_segment(CP_FileSegment *segment, uint32_t *file_id, uint32_t *segment_number, char *data, uint16_t *segment_size) {
    *file_id = segment->file_id;
    *segment_number = segment->segment_number;
    *segment_size = segment->segment_size;
    memcpy(data, segment->data, *segment_size);
}

void encode_file_segment_ack(CP_FileSegmentAck *ack, uint32_t file_id, uint32_t segment_number) {
    ack->header.type = CP_FILE_SEGMENT_ACK;
    ack->header.length = sizeof(CP_FileSegmentAck) - sizeof(CP_Header);
    ack->file_id = file_id;
    ack->segment_number = segment_number;
}

void decode_file_segment_ack(CP_FileSegmentAck *ack, uint32_t *file_id, uint32_t *segment_number) {
    *file_id = ack->file_id;
    *segment_number = ack->segment_number;
}

void encode_file_transfer_complete(CP_FileTransferComplete *complete, uint32_t file_id) {
    complete->header.type = CP_FILE_TRANSFER_COMPLETE;
    complete->header.length = sizeof(CP_FileTransferComplete) - sizeof(CP_Header);
    complete->file_id = file_id;
}

void decode_file_transfer_complete(CP_FileTransferComplete *complete, uint32_t *file_id) {
    *file_id = complete->file_id;
}
