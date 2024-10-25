#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#define PORT 8080
#define USERS_FILE "users.txt" // File to store user credentials
#define TRANSACTION_FILE "transactions.txt"
#include <time.h> // For timestamps

void log_transaction(const char* username, const char* transaction_type, int amount) {
    FILE *file = fopen(TRANSACTION_FILE, "a");
    if (!file) {
        perror("Error opening transaction file");
        return;
    }
    // Get current timestamp
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char timestamp[50];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", t);

    // Log transaction details
    fprintf(file, "%s %s %d %s\n", username, transaction_type, amount, timestamp);
    fclose(file);
}

// Handle customer operations
void handle_customer_operations(int client_sock, char* username) {
    char buffer[1024] = {0}, operation[50];
    int balance, amount;
    FILE *file, *temp_file;
    char line[150], file_username[50], file_password[50];
    int user_found = 0;

    send(client_sock, "1. View Balance\n2. Deposit\n3. Withdraw\n4. Transfer Funds\n5. Apply for Loan\n6. View Transaction History\n7. Add Feedback\n8. Logout\n", 190, 0);
    recv(client_sock, operation, sizeof(operation), 0);

    // Open customer file for reading
    file = fopen("customer.txt", "r");
    if (!file) {
        send(client_sock, "Error opening customer file.\n", 30, 0);
        return;
    }

    // Open a temp file for writing
    temp_file = fopen("temp_customer.txt", "w");
    if (!temp_file) {
        fclose(file);
        send(client_sock, "Error opening temp file.\n", 25, 0);
        return;
    }

    while (fgets(line, sizeof(line), file)) {
        sscanf(line, "%49s %49s %d", file_username, file_password, &balance);
        if (strcmp(file_username, username) == 0) {
            user_found = 1;
            if (strcmp(operation, "1") == 0) {
                // View account balance
                sprintf(buffer, "Your balance is: %d\n", balance);
                send(client_sock, buffer, strlen(buffer), 0);
            } else if (strcmp(operation, "2") == 0) {
                // Deposit money
                send(client_sock, "Enter deposit amount:\n", 22, 0);
                recv(client_sock, buffer, sizeof(buffer), 0);
                amount = atoi(buffer);
                balance += amount;
                log_transaction(username, "Deposit", amount); // Log transaction
                send(client_sock, "Deposit successful.\n", 20, 0);
            } else if (strcmp(operation, "3") == 0) {
                // Withdraw money
                send(client_sock, "Enter withdrawal amount:\n", 25, 0);
                recv(client_sock, buffer, sizeof(buffer), 0);
                amount = atoi(buffer);
                if (amount <= balance) {
                    balance -= amount;
                    log_transaction(username, "Withdrawal", amount); // Log transaction
                    send(client_sock, "Withdrawal successful.\n", 23, 0);
                } else {
                    send(client_sock, "Insufficient funds.\n", 20, 0);
                }
            } else if (strcmp(operation, "4") == 0) {
                // Transfer funds
                char recipient[50];
                send(client_sock, "Enter recipient username:\n", 27, 0);
                recv(client_sock, recipient, sizeof(recipient), 0);

                send(client_sock, "Enter transfer amount:\n", 24, 0);
                recv(client_sock, buffer, sizeof(buffer), 0);
                amount = atoi(buffer);

                if (amount <= balance) {
                    balance -= amount;
                    // Update recipient's balance
                    FILE *temp = fopen("customer.txt", "r");
                    FILE *updated_file = fopen("temp_update.txt", "w");
                    int recipient_found = 0;
                    while (fgets(line, sizeof(line), temp)) {
                        sscanf(line, "%49s %49s %d", file_username, file_password, &balance);
                        if (strcmp(file_username, recipient) == 0) {
                            balance += amount;
                            recipient_found = 1;
                        }
                        fprintf(updated_file, "%s %s %d\n", file_username, file_password, balance);
                    }
                    fclose(temp);
                    fclose(updated_file);
                    if (recipient_found) {
                        remove("customer.txt");
                        rename("temp_update.txt", "customer.txt");
                        log_transaction(username, "Transfer", amount); // Log transaction
                        send(client_sock, "Transfer successful.\n", 21, 0);
                    } else {
                        send(client_sock, "Recipient not found.\n", 21, 0);
                    }
                } else {
                    send(client_sock, "Insufficient funds.\n", 20, 0);
                }
            } else if (strcmp(operation, "5") == 0) {
                // Apply for loan
                send(client_sock, "Loan application submitted.\n", 29, 0);
                log_transaction(username, "Loan Application", 0);
            } else if (strcmp(operation, "6") == 0) {
                // View transaction history
                FILE *trans_file = fopen(TRANSACTION_FILE, "r");
                if (!trans_file) {
                    send(client_sock, "Error opening transaction history.\n", 36, 0);
                    return;
                }

                char trans_line[200], trans_username[50], trans_type[50], trans_timestamp[50];
                int trans_amount;
                char history[2000] = "";
                int found = 0;

                while (fgets(trans_line, sizeof(trans_line), trans_file)) {
                    sscanf(trans_line, "%49s %49s %d %49[^\n]", trans_username, trans_type, &trans_amount, trans_timestamp);
                    if (strcmp(trans_username, username) == 0) {
                        sprintf(buffer, "Type: %s, Amount: %d, Date: %s\n", trans_type, trans_amount, trans_timestamp);
                        strcat(history, buffer);
                        found = 1;
                    }
                }

                fclose(trans_file);

                if (found) {
                    send(client_sock, history, strlen(history), 0);
                } else {
                    send(client_sock, "No transaction history found.\n", 30, 0);
                }
            } else if (strcmp(operation, "7") == 0) {
                // Add feedback (simple placeholder)
                send(client_sock, "Feedback submitted successfully.\n", 33, 0);
            }
            // Update user balance in temp file
            fprintf(temp_file, "%s %s %d\n", file_username, file_password, balance);
        } else {
            // Write other users' info unchanged
            fprintf(temp_file, "%s", line);
        }
    }

    fclose(file);
    fclose(temp_file);

    // Replace the old customer file with the updated file
    if (user_found) {
        remove("customer.txt");
        rename("temp_customer.txt", "customer.txt");
    } else {
        remove("temp_customer.txt");
        send(client_sock, "User not found.\n", 17, 0);
    }
}

// Function to verify credentials 
int verify_credentials(const char *username, const char *password, const char *role) {
    FILE *file = fopen(USERS_FILE, "r");
    if (file == NULL) {
        perror("Error opening users file");
        return 0;
    }

    char file_username[50], file_password[50], file_role[20];
    
    while (fscanf(file, "%49s %49s %19s", file_username, file_password, file_role) == 3) {
        if (strcmp(username, file_username) == 0 && 
            strcmp(password, file_password) == 0 && 
            strcmp(role, file_role) == 0) {
            fclose(file);
            return 1;  // Credentials match
        }
    }

    fclose(file);
    return 0;  // No match found
}

// Admin functionalities
void add_user(const char *new_username, const char *new_password, const char *new_role) {
    FILE *file = fopen(USERS_FILE, "a");  // Append to file
    if (file == NULL) {
        perror("Error opening users file");
        return;
    }
    fprintf(file, "%s %s %s\n", new_username, new_password, new_role);  // Write new user to file
    fclose(file);
}

void delete_user(const char *del_username) {
    FILE *file = fopen(USERS_FILE, "r");
    FILE *temp = fopen("temp.txt", "w");
    if (file == NULL || temp == NULL) {
        perror("Error opening file");
        return;
    }

    char file_username[50], file_password[50], file_role[20];
    
    // Copy all users except the one to be deleted
    while (fscanf(file, "%49s %49s %19s", file_username, file_password, file_role) == 3) {
        if (strcmp(file_username, del_username) != 0) {
            fprintf(temp, "%s %s %s\n", file_username, file_password, file_role);
        }
    }

    fclose(file);
    fclose(temp);
    remove(USERS_FILE);           // Delete old users file
    rename("temp.txt", USERS_FILE);  // Rename temp file to users.txt
}

void view_users(int client_sock) {
    FILE *file = fopen(USERS_FILE, "r");
    if (file == NULL) {
        perror("Error opening users file");
        return;
    }

    char file_username[50], file_password[50], file_role[20];
    char user_info[1024] = "";

    // Read all users and send to the client
    while (fscanf(file, "%49s %49s %19s", file_username, file_password, file_role) == 3) {
        char line[200];
        snprintf(line, sizeof(line), "Username: %s, Role: %s\n", file_username, file_role);
        strcat(user_info, line);  // Append user info to string
    }
    
    send(client_sock, user_info, strlen(user_info), 0);  // Send users list to client
    fclose(file);
}


int main() {
    int server_fd, client_sock;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};

    // Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        return 1;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind socket
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        return 1;
    }

    // Listen for incoming connections
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        return 1;
    }

    printf("Server listening on port %d...\n", PORT);

    if ((client_sock = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
        perror("Accept failed");
        return 1;
    }

    // Read login credentials
    read(client_sock, buffer, 1024);

    char username[50], password[50], role[20];
    sscanf(buffer, "%49s %49s %19s", username, password, role);

    // Verify credentials
    if (verify_credentials(username, password, role)) {
        send(client_sock, "Login successful!", 18, 0);

        if (strcmp(role, "admin") == 0) {
            // Admin menu handling
            char admin_command[1024];
            while (1) {
                read(client_sock, admin_command, 1024);  // Receive admin command from client

                if (strncmp(admin_command, "ADD", 3) == 0) {
                    char new_username[50], new_password[50], new_role[20];
                    sscanf(admin_command + 4, "%49s %49s %19s", new_username, new_password, new_role);
                    add_user(new_username, new_password, new_role);
                    send(client_sock, "User added successfully!", 25, 0);
                } 
                else if (strncmp(admin_command, "DELETE", 6) == 0) {
                    char del_username[50];
                    sscanf(admin_command + 7, "%49s", del_username);
                    delete_user(del_username);
                    send(client_sock, "User deleted successfully!", 27, 0);
                } 
                else if (strncmp(admin_command, "VIEW", 4) == 0) {
                    view_users(client_sock);
                } 
                else if (strncmp(admin_command, "EXIT", 4) == 0) {
                    break;  // Exit the admin session
                }
            }
}
else if (strcmp(role, "customer") == 0) {
            handle_customer_operations(client_sock, username);  // Call the function here
        }
    }else {
        send(client_sock, "Login failed!", 13, 0);
    }

    close(client_sock);
    close(server_fd);
    return 0;
}
