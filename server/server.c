#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <json-c/json.h>
#include "message.h"
#include "states.h"

#define MAX_CONN 1024
#define MAX_FILE_ID 1000

typedef struct {
    uint32_t file_id;
    FILE *file;
    uint64_t file_size;
    uint32_t received_segments;
} FileTransfer;

FileTransfer file_transfers[MAX_FILE_ID];

void handle_file_transfer_request(int sockfd, struct sockaddr_in *cliaddr, socklen_t len, CP_FileTransferRequest *request);
void handle_file_segment(int sockfd, struct sockaddr_in *cliaddr, socklen_t len, CP_FileSegment *segment);
void handle_file_segment_ack(int sockfd, struct sockaddr_in *cliaddr, socklen_t len, CP_FileSegmentAck *ack);
void handle_file_transfer_complete(int sockfd, struct sockaddr_in *cliaddr, socklen_t len, CP_FileTransferComplete *complete);

void read_server_config(const char *filename, int *port) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        printf("Failed to open server configuration file: %s\n", strerror(errno));
        handle_transition(ERROR);
        exit(EXIT_FAILURE);
    }

    struct json_object *parsed_json;
    struct json_object *json_port;

    char buffer[256];
    fread(buffer, sizeof(buffer), 1, file);
    fclose(file);

    parsed_json = json_tokener_parse(buffer);
    json_object_object_get_ex(parsed_json, "port", &json_port);
    *port = json_object_get_int(json_port);
}

int main() {
    int sockfd;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t len;
    char buffer[MAX_MESSAGE_SIZE + sizeof(CP_Header)];
    CP_TextMessage text_message;
    CP_FileTransferRequest file_request;
    CP_FileSegment file_segment;
    CP_FileSegmentAck file_ack;
    CP_FileTransferComplete file_complete;
    char decoded_message[MAX_MESSAGE_SIZE];

    current_state = DISCONNECTED;
    handle_transition(CONNECTING);

    int port = 4433;
    read_server_config("config/server_config.json", &port);

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        handle_transition(ERROR);
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(port);

    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("Bind failed");
        handle_transition(ERROR);
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    handle_transition(CONNECTED);
    printf("Server listening on port %d\n", port);

    while (1) {
        len = sizeof(cliaddr);
        int n = recvfrom(sockfd, buffer, sizeof(buffer), MSG_WAITALL, (struct sockaddr *)&cliaddr, &len);
        if (n > 0) {
            printf("Received %d bytes\n", n);
            CP_Header *header = (CP_Header *)buffer;
            switch (header->type) {
                case CP_TEXT_MESSAGE: // Text Message
                    memcpy(&text_message, buffer, sizeof(text_message));
                    decode_text_message(&text_message, decoded_message);
                    printf("Received message: %s\n", decoded_message);
                    sendto(sockfd, buffer, n, MSG_CONFIRM, (const struct sockaddr *)&cliaddr, len);
                    break;
                case CP_FILE_TRANSFER_REQUEST: // File Transfer Request
                    memcpy(&file_request, buffer, sizeof(file_request));
                    handle_file_transfer_request(sockfd, &cliaddr, len, &file_request);
                    break;
                case CP_FILE_SEGMENT: // File Segment
                    memcpy(&file_segment, buffer, sizeof(file_segment));
                    handle_file_segment(sockfd, &cliaddr, len, &file_segment);
                    break;
                case CP_FILE_SEGMENT_ACK: // File Segment Ack
                    memcpy(&file_ack, buffer, sizeof(file_ack));
                    handle_file_segment_ack(sockfd, &cliaddr, len, &file_ack);
                    break;
                case CP_FILE_TRANSFER_COMPLETE: // File Transfer Complete
                    memcpy(&file_complete, buffer, sizeof(file_complete));
                    handle_file_transfer_complete(sockfd, &cliaddr, len, &file_complete);
                    break;
                default:
                    printf("Unknown message type: %d\n", header->type);
                    handle_transition(ERROR);
                    break;
            }
        } else {
            handle_transition(ERROR);
        }
    }

    handle_transition(DISCONNECTING);
    close(sockfd);
    handle_transition(DISCONNECTED);
    return 0;
}

void handle_file_transfer_request(int sockfd, struct sockaddr_in *cliaddr, socklen_t len, CP_FileTransferRequest *request) {
    static uint32_t current_file_id = 1;
    char filename[MAX_FILENAME_LENGTH];
    uint64_t file_size;

    decode_file_transfer_request(request, filename, &file_size);

    if (current_file_id >= MAX_FILE_ID) {
        printf("Maximum number of file transfers reached\n");
        handle_transition(ERROR);
        return;
    }

    FileTransfer *transfer = &file_transfers[current_file_id];
    transfer->file_id = current_file_id;
    transfer->file_size = file_size;
    transfer->received_segments = 0;

    char full_filename[MAX_FILENAME_LENGTH + 10];
    snprintf(full_filename, sizeof(full_filename), "%u_%s", current_file_id, filename);

    transfer->file = fopen(full_filename, "wb");
    if (transfer->file == NULL) {
        perror("Failed to open file for writing");
        handle_transition(ERROR);
        return;
    }

    printf("File transfer initiated: %s (ID: %u, Size: %" PRIu64 " bytes)\n", filename, current_file_id, file_size);

    current_file_id++;
}

void handle_file_segment(int sockfd, struct sockaddr_in *cliaddr, socklen_t len, CP_FileSegment *segment) {
    uint32_t file_id, segment_number;
    uint16_t segment_size;
    char segment_data[FILE_SEGMENT_SIZE];

    decode_file_segment(segment, &file_id, &segment_number, segment_data, &segment_size);

    if (file_id >= MAX_FILE_ID || file_transfers[file_id].file == NULL) {
        printf("Invalid file ID: %u\n", file_id);
        handle_transition(ERROR);
        return;
    }

    FileTransfer *transfer = &file_transfers[file_id];
    fwrite(segment_data, 1, segment_size, transfer->file);
    transfer->received_segments++;

    printf("Received segment %u of file ID %u\n", segment_number, file_id);

    CP_FileSegmentAck ack;
    encode_file_segment_ack(&ack, file_id, segment_number);
    sendto(sockfd, &ack, sizeof(ack), MSG_CONFIRM, (const struct sockaddr *)cliaddr, len);
}

void handle_file_segment_ack(int sockfd, struct sockaddr_in *cliaddr, socklen_t len, CP_FileSegmentAck *ack) {
    uint32_t file_id, segment_number;

    decode_file_segment_ack(ack, &file_id, &segment_number);

    printf("Acknowledgment received for segment %u of file ID %u\n", segment_number, file_id);
}

void handle_file_transfer_complete(int sockfd, struct sockaddr_in *cliaddr, socklen_t len, CP_FileTransferComplete *complete) {
    uint32_t file_id;

    decode_file_transfer_complete(complete, &file_id);

    if (file_id >= MAX_FILE_ID || file_transfers[file_id].file == NULL) {
        printf("Invalid file ID: %u\n", file_id);
        handle_transition(ERROR);
        return;
    }

    FileTransfer *transfer = &file_transfers[file_id];
    fclose(transfer->file);
    transfer->file = NULL;

    printf("File transfer complete: ID %u\n", file_id);
}
