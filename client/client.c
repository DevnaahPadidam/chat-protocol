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

// Define maximum message and file segment sizes
#define MAX_MESSAGE_SIZE 1024
#define FILE_SEGMENT_SIZE 512

// Function declarations for sending file transfer requests and segments
void send_file_transfer_request(int sockfd, struct sockaddr_in *servaddr, socklen_t len, const char *filename);
void send_file_segments(int sockfd, struct sockaddr_in *servaddr, socklen_t len, FILE *file, uint32_t file_id);

// Function to read client configuration from a JSON file
void read_client_config(const char *filename, char *server_ip, int *port) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        printf("Failed to open client configuration file: %s\n", strerror(errno));
        handle_transition(ERROR);
        exit(EXIT_FAILURE);
    }

    struct json_object *parsed_json;
    struct json_object *json_server_ip;
    struct json_object *json_port;

    char buffer[256];
    fread(buffer, sizeof(buffer), 1, file);
    fclose(file);

    parsed_json = json_tokener_parse(buffer);
    json_object_object_get_ex(parsed_json, "server_ip", &json_server_ip);
    json_object_object_get_ex(parsed_json, "server_port", &json_port);
    strcpy(server_ip, json_object_get_string(json_server_ip));
    *port = json_object_get_int(json_port);
}

int main() {
    int sockfd;
    struct sockaddr_in servaddr;
    socklen_t len;
    char buffer[MAX_MESSAGE_SIZE + sizeof(CP_Header)];
    CP_TextMessage text_message;
    CP_FileTransferRequest file_request;
    CP_FileSegment file_segment;
    CP_FileSegmentAck file_ack;
    CP_FileTransferComplete file_complete;
    char input[MAX_MESSAGE_SIZE];
    char decoded_message[MAX_MESSAGE_SIZE];
    char server_ip[16];
    int port;

    // Initialize state to DISCONNECTED and transition to CONNECTING
    current_state = DISCONNECTED;
    handle_transition(CONNECTING);

    // Read client configuration to get server IP and port
    read_client_config("config/client_config.json", server_ip, &port);

    // Create a socket for communication
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        handle_transition(ERROR);
        exit(EXIT_FAILURE);
    }

    // Initialize server address structure
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);

    // Convert server IP address from text to binary form
    if (inet_pton(AF_INET, server_ip, &servaddr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        handle_transition(ERROR);
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Transition to CONNECTED state
    handle_transition(CONNECTED);
    len = sizeof(servaddr);

    while (1) {
        // Prompt user for input (message or filename)
        printf("Enter message or filename: ");
        fgets(input, sizeof(input), stdin);
        input[strcspn(input, "\n")] = '\0'; // Remove newline character

        if (strncmp(input, "file:", 5) == 0) {
            // Handle file transfer request
            const char *filename = input + 5;
            send_file_transfer_request(sockfd, &servaddr, len, filename);
        } else {
            // Handle text message
            encode_text_message(&text_message, input);
            memcpy(buffer, &text_message, sizeof(text_message));

            sendto(sockfd, buffer, sizeof(buffer), MSG_CONFIRM, (const struct sockaddr *)&servaddr, len);

            printf("Sending message: %s\n", input);
            printf("Bytes sent: %ld\n", sizeof(buffer));

            // Receive server response
            int n = recvfrom(sockfd, buffer, sizeof(buffer), MSG_WAITALL, (struct sockaddr *)&servaddr, &len);
            if (n > 0) {
                memcpy(&text_message, buffer, sizeof(text_message));
                decode_text_message(&text_message, decoded_message);
                printf("Server echo: %s\n", decoded_message);
            } else {
                handle_transition(ERROR);
            }
        }
    }

    // Transition to DISCONNECTING state and close the socket
    handle_transition(DISCONNECTING);
    close(sockfd);
    handle_transition(DISCONNECTED);
    return 0;
}

// Function to send a file transfer request to the server
void send_file_transfer_request(int sockfd, struct sockaddr_in *servaddr, socklen_t len, const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        perror("Failed to open file");
        handle_transition(ERROR);
        return;
    }

    // Get the file size
    fseek(file, 0, SEEK_END);
    uint64_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Encode and send the file transfer request
    CP_FileTransferRequest request;
    encode_file_transfer_request(&request, filename, file_size);
    sendto(sockfd, &request, sizeof(request), MSG_CONFIRM, (const struct sockaddr *)servaddr, len);

    printf("File transfer request sent: %s (Size: %" PRIu64 " bytes)\n", filename, file_size);

    // Send the file segments
    send_file_segments(sockfd, servaddr, len, file, request.header.length);
    fclose(file);
}

// Function to send file segments to the server
void send_file_segments(int sockfd, struct sockaddr_in *servaddr, socklen_t len, FILE *file, uint32_t file_id) {
    uint32_t segment_number = 0;
    char segment_data[FILE_SEGMENT_SIZE];
    size_t bytes_read;

    // Read and send file segments
    while ((bytes_read = fread(segment_data, 1, FILE_SEGMENT_SIZE, file)) > 0) {
        CP_FileSegment segment;
        encode_file_segment(&segment, file_id, segment_number, segment_data, bytes_read);
        sendto(sockfd, &segment, sizeof(segment), MSG_CONFIRM, (const struct sockaddr *)servaddr, len);

        printf("File segment %u sent (Size: %zu bytes)\n", segment_number, bytes_read);

        // Receive acknowledgment for the sent segment
        char buffer[sizeof(CP_FileSegmentAck)];
        int n = recvfrom(sockfd, buffer, sizeof(buffer), MSG_WAITALL, (struct sockaddr *)servaddr, &len);
        if (n > 0) {
            CP_FileSegmentAck ack;
            memcpy(&ack, buffer, sizeof(ack));
            uint32_t ack_file_id, ack_segment_number;
            decode_file_segment_ack(&ack, &ack_file_id, &ack_segment_number);

            if (ack_file_id == file_id && ack_segment_number == segment_number) {
                printf("Acknowledgment received for segment %u\n", segment_number);
            } else {
                printf("Invalid acknowledgment received\n");
                handle_transition(ERROR);
                return;
            }
        } else {
            handle_transition(ERROR);
            return;
        }

        segment_number++;
    }

    // Send file transfer complete message
    CP_FileTransferComplete complete;
    encode_file_transfer_complete(&complete, file_id);
    sendto(sockfd, &complete, sizeof(complete), MSG_CONFIRM, (const struct sockaddr *)servaddr, len);

    printf("File transfer complete: ID %u\n", file_id);
}
