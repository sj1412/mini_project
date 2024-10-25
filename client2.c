#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#define PORT 8080
void customer_menu(int sock) {
  int valread;
  char buffer[1024]={0};
    char option[10];
while (1) {
        valread = read(sock, buffer, 1024); // Read operations menu from server
        printf("%s\n", buffer);

        printf("Select an option: ");
        scanf("%s", option);
        send(sock, option, strlen(option), 0); // Send selected option to server

        if (strcmp(option, "8") == 0) {
            printf("Logging out...\n");
            break;  // Exit the loop on logout
        }

        valread = read(sock, buffer, 1024); // Read response from server
        printf("%s\n", buffer);
    }
}
void admin_menu(int sock) {
    char command[1024];
    while (1) {
        printf("\nAdmin Menu:\n");
        printf("1. Add User\n");
        printf("2. Delete User\n");
        printf("3. View Users\n");
        printf("4. Exit\n");
        printf("Enter choice: ");
        int choice;
        scanf("%d", &choice);

        if (choice == 1) {
            char new_username[50], new_password[50], new_role[20];
            printf("Enter new username: ");
            scanf("%49s", new_username);
            printf("Enter new password: ");
            scanf("%49s", new_password);
            printf("Enter role (customer/employee/manager): ");
            scanf("%19s", new_role);
            snprintf(command, sizeof(command), "ADD %s %s %s", new_username, new_password, new_role);
            send(sock, command, strlen(command), 0);
            read(sock, command, 1024);
            printf("Server: %s\n", command);
        } 
        else if (choice == 2) {
            char del_username[50];
            printf("Enter username to delete: ");
            scanf("%49s", del_username);
            snprintf(command, sizeof(command), "DELETE %s", del_username);
            send(sock, command, strlen(command), 0);
            read(sock, command, 1024);
            printf("Server: %s\n", command);
        } 
        else if (choice == 3) {
            send(sock, "VIEW", 4, 0);
            read(sock, command, 1024);
            printf("Server:\n%s", command);
        } 
        else if (choice == 4) {
            send(sock, "EXIT", 4, 0);
            break;
        }
    }
}

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};
    char username[50], password[50], role[20];

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Socket creation error\n");
        return 1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("Invalid address\n");
        return 1;
    }

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("Connection failed\n");
        return 1;
    }

    // Get login details
    printf("Enter username: ");
    scanf("%49s", username);
    printf("Enter password: ");
    scanf("%49s", password);
    printf("Enter role (admin/customer/employee/manager): ");
    scanf("%19s", role);

    // Send login details to server
    snprintf(buffer, sizeof(buffer), "%s %s %s", username, password, role);
    send(sock, buffer, strlen(buffer), 0);

    // Receive server response
    read(sock, buffer, 1024);
    printf("Server: %s\n", buffer);

    // If admin, display admin menu
    if (strcmp(role, "admin") == 0 && strcmp(buffer, "Login successful!") == 0) {
        admin_menu(sock);
    }
if (strcmp(role, "customer") == 0 && strcmp(buffer, "Login successful!") == 0) {
       customer_menu(sock);
    }
    close(sock);
    return 0;
}

