#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_CODE_SIZE 10000
#define MAX_TOKEN_SIZE 256
#define MAX_VARIABLES 1000
#define MAX_EXPR_DEPTH 100
#define HASH_SIZE 101

typedef enum {
    T_PRINT, T_SET, T_LOOP, T_IF,
    T_NUMBER, T_STRING, T_ID,
    T_LPAREN, T_RPAREN, T_LBRACE, T_RBRACE, T_SEMICOLON,
    T_EQUALS, T_PLUS, T_MINUS, T_MULTIPLY, T_DIVIDE, 
    T_MODULO, T_LESS, T_GREATER, T_EQUAL_COMPARE,
    T_AND, T_OR, T_NOT,
    T_EOF
} TokenType;

typedef struct Token {
    TokenType type;
    union {
        int number;
        char* string;
        struct Token* expr;
    } value;
    struct Token* next;
} Token;

typedef struct Variable {
    char* name;
    int value;
    struct Variable* next;
} Variable;

static Variable* variables[HASH_SIZE] = {0};
static char* input;
static size_t input_len;
static size_t pos = 0;
static Token* current_token;
static char token_buffer[MAX_TOKEN_SIZE];

unsigned int hash(const char* str);
int get_variable(const char* name);
void set_variable(const char* name, int value);
void error(const char* msg);
Token* get_next_token(void);
void execute(void);
void cleanup_variables(void);
Token* parse_expression(void);
int evaluate_expression(Token* expr);
void free_token_list(Token* token);

unsigned int hash(const char* str) {
    unsigned int hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash % HASH_SIZE;
}

int get_variable(const char* name) {
    unsigned int h = hash(name);
    Variable* var = variables[h];
    while (var) {
        if (strcmp(var->name, name) == 0) {
            return var->value;
        }
        var = var->next;
    }
    error("Undefined variable");
    return 0;
}

void set_variable(const char* name, int value) {
    unsigned int h = hash(name);
    Variable* var = variables[h];
    
    while (var) {
        if (strcmp(var->name, name) == 0) {
            var->value = value;
            return;
        }
        var = var->next;
    }
    
    var = malloc(sizeof(Variable));
    if (!var) {
        error("Memory allocation failed for variable");
    }
    var->name = strdup(name);
    var->value = value;
    var->next = variables[h];
    variables[h] = var;
}

void error(const char* msg) {
    fprintf(stderr, "[Error]: %s\n", msg);
    fprintf(stderr, "At position: %zu\n", pos);
    cleanup_variables();
    exit(1);
}

Token* get_next_token(void) {
    Token* token = malloc(sizeof(Token));
    memset(token, 0, sizeof(Token));

    while (pos < input_len && isspace(input[pos])) {
        pos++;
    }

    if (pos >= input_len) {
        token->type = T_EOF;
        return token;
    }

    char c = input[pos];

    if (isdigit(c)) {
        int num = 0;
        while (pos < input_len && isdigit(input[pos])) {
            num = num * 10 + (input[pos] - '0');
            pos++;
        }
        token->type = T_NUMBER;
        token->value.number = num;
        return token;
    }

    if (c == '"') {
        pos++;  
        int i = 0;
        while (pos < input_len && input[pos] != '"') {
            token_buffer[i++] = input[pos++];
            if (i >= MAX_TOKEN_SIZE - 1) {
                error("String too long");
            }
        }
        token_buffer[i] = '\0';
        if (pos < input_len && input[pos] == '"') {
            pos++; 
        } else {
            error("Unterminated string");
        }
        token->type = T_STRING;
        token->value.string = strdup(token_buffer);
        return token;
    }

    if (isalpha(c)) {
        int i = 0;
        while (pos < input_len && (isalnum(input[pos]) || input[pos] == '_')) {
            token_buffer[i++] = input[pos++];
            if (i >= MAX_TOKEN_SIZE - 1) {
                error("Identifier too long");
            }
        }
        token_buffer[i] = '\0';

        if (strcmp(token_buffer, "yap") == 0) token->type = T_PRINT;
        else if (strcmp(token_buffer, "sigma") == 0) token->type = T_SET;
        else if (strcmp(token_buffer, "fr_fr") == 0) token->type = T_LOOP;
        else if (strcmp(token_buffer, "based") == 0) token->type = T_IF;
        else {
            token->type = T_ID;
            token->value.string = strdup(token_buffer);
        }
        return token;
    }

    pos++;
    switch (c) {
        case '(': token->type = T_LPAREN; break;
        case ')': token->type = T_RPAREN; break;
        case '{': token->type = T_LBRACE; break;
        case '}': token->type = T_RBRACE; break;
        case ';': token->type = T_SEMICOLON; break;
        case '=': 
            if (pos < input_len && input[pos] == '=') {
                pos++;
                token->type = T_EQUAL_COMPARE;
            } else {
                token->type = T_EQUALS;
            }
            break;
        case '+': token->type = T_PLUS; break;
        case '-': token->type = T_MINUS; break;
        case '*': token->type = T_MULTIPLY; break;
        case '/': token->type = T_DIVIDE; break;
        case '%': token->type = T_MODULO; break;
        case '<': token->type = T_LESS; break;
        case '>': token->type = T_GREATER; break;
        default: {
            char error_msg[100];
            snprintf(error_msg, sizeof(error_msg), "Unknown character: %c", c);
            error(error_msg);
        }
    }
    
    return token;
}

Token* parse_expression(void) {
    Token* expr_head = NULL;
    Token* current = NULL;

    while (current_token->type != T_SEMICOLON && 
           current_token->type != T_RPAREN && 
           current_token->type != T_EOF) {
        
        Token* new_token = malloc(sizeof(Token));
        memcpy(new_token, current_token, sizeof(Token));
        new_token->next = NULL;

        if (!expr_head) {
            expr_head = new_token;
            current = new_token;
        } else {
            current->next = new_token;
            current = new_token;
        }

        current_token = get_next_token();
    }

    return expr_head;
}

int evaluate_expression(Token* expr) {
    int result = 0;
    Token* current = expr;
    Token* prev = NULL;
    TokenType last_op = T_EOF;

    while (current) {
        switch (current->type) {
            case T_NUMBER:
                if (last_op == T_PLUS) result += current->value.number;
                else if (last_op == T_MINUS) result -= current->value.number;
                else if (last_op == T_MULTIPLY) result *= current->value.number;
                else if (last_op == T_DIVIDE) {
                    if (current->value.number == 0) error("Division by zero");
                    result /= current->value.number;
                }
                else if (last_op == T_MODULO) {
                    if (current->value.number == 0) error("Modulo by zero");
                    result %= current->value.number;
                }
                else if (last_op == T_EOF) result = current->value.number;
                last_op = T_EOF;
                break;
            
            case T_ID:
                if (last_op == T_PLUS) result += get_variable(current->value.string);
                else if (last_op == T_MINUS) result -= get_variable(current->value.string);
                else if (last_op == T_MULTIPLY) result *= get_variable(current->value.string);
                else if (last_op == T_DIVIDE) {
                    int var_val = get_variable(current->value.string);
                    if (var_val == 0) error("Division by zero");
                    result /= var_val;
                }
                else if (last_op == T_MODULO) {
                    int var_val = get_variable(current->value.string);
                    if (var_val == 0) error("Modulo by zero");
                    result %= var_val;
                }
                else if (last_op == T_EOF) result = get_variable(current->value.string);
                last_op = T_EOF;
                break;

            case T_PLUS:
            case T_MINUS:
            case T_MULTIPLY:
            case T_DIVIDE:
            case T_MODULO:
                last_op = current->type;
                break;

            default:
                error("Invalid token in expression");
        }
        current = current->next;
    }

    return result;
}

void free_token_list(Token* token) {
    while (token) {
        Token* next = token->next;
        if (token->type == T_STRING || token->type == T_ID) {
            free(token->value.string);
        }
        free(token);
        token = next;
    }
}

void execute(void) {
    while (current_token->type != T_EOF) {
        switch (current_token->type) {
            case T_PRINT: {
                current_token = get_next_token(); 
                
                if (current_token->type != T_LPAREN) {
                    error("Expected '(' after 'hello_world'");
                }
                
                current_token = get_next_token();  
                
                if (current_token->type == T_STRING) {
                    printf("%s\n", current_token->value.string);
                } else {
                    Token* expr = parse_expression();
                    int result = evaluate_expression(expr);
                    printf("%d\n", result);
                    free_token_list(expr);
                }
                
                if (current_token->type != T_RPAREN) {
                    error("Expected ')' after print argument");
                }
                
                current_token = get_next_token(); 
                
                if (current_token->type != T_SEMICOLON) {
                    error("Expected ';' after print statement");
                }
                
                current_token = get_next_token(); 
                break;
            }
            
            case T_SET: {
                current_token = get_next_token();  
                
                if (current_token->type != T_ID) {
                    error("Expected variable name after 'sigma'");
                }
                
                char* var_name = current_token->value.string;
                
                current_token = get_next_token();  
                
                if (current_token->type != T_EQUALS) {
                    error("Expected '=' after variable name");
                }
                
                current_token = get_next_token();  
                
                Token* expr = parse_expression();
                int value = evaluate_expression(expr);
                free_token_list(expr);
                
                set_variable(var_name, value);
                
                if (current_token->type != T_SEMICOLON) {
                    error("Expected ';' after assignment");
                }
                
                current_token = get_next_token();  
                break;
            }
            
            case T_LOOP: {
                current_token = get_next_token();
                
                if (current_token->type != T_LPAREN) {
                    error("Expected '(' after 'fr_fr'");
                }
                
                current_token = get_next_token();
                Token* condition_expr = parse_expression();
                
                if (current_token->type != T_LBRACE) {
                    error("Expected '{' after loop condition");
                }
                
                current_token = get_next_token();
                
                while (evaluate_expression(condition_expr)) {
                    Token* loop_body = current_token;
                    
                    while (loop_body->type != T_RBRACE) {
                        current_token = loop_body;
                        execute();
                        loop_body = current_token;
                    }
                    
                    // Reevaluate condition
                    free_token_list(condition_expr);
                    current_token = get_next_token();
                    condition_expr = parse_expression();
                }
                
                free_token_list(condition_expr);
                
                if (current_token->type != T_RBRACE) {
                    error("Expected '}' to close loop");
                }
                
                current_token = get_next_token();
                break;
            }
            
            case T_IF: {
                current_token = get_next_token();
                
                if (current_token->type != T_LPAREN) {
                    error("Expected '(' after 'based'");
                }
                
                current_token = get_next_token();
                Token* condition_expr = parse_expression();
                int condition_result = evaluate_expression(condition_expr);
                free_token_list(condition_expr);
                
                if (current_token->type != T_LBRACE) {
                    error("Expected '{' after if condition");
                }
                
                current_token = get_next_token();
                
                if (condition_result) {
                    while (current_token->type != T_RBRACE) {
                        execute();
                    }
                } else {
                    while (current_token->type != T_RBRACE) {
                        current_token = get_next_token();
                    }
                }
                
                current_token = get_next_token();
                break;
            }
            
            default:
                error("Why are you gay!!!");
        }
    }
}
void cleanup_variables(void) {
    for (int i = 0; i < HASH_SIZE; i++) {
        Variable* current = variables[i];
        while (current != NULL) {
            Variable* next = current->next;
            free(current->name);
            free(current);
            current = next;
        }
        variables[i] = NULL;
    }
    
    if (input) {
        free(input);
        input = NULL;
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    FILE* file = fopen(argv[1], "r");
    if (!file) {
        fprintf(stderr, "Error opening file %s\n", argv[1]);
        return 1;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    fprintf(stderr, "File size: %ld bytes\n", file_size);

    input = malloc(file_size + 16);  
    if (!input) {
        fprintf(stderr, "Memory allocation failed\n");
        fclose(file);
        return 1;
    }

    size_t bytes_read = fread(input, 1, file_size, file);
    
    if (bytes_read < file_size) {
        rewind(file);
        bytes_read = 0;
        char line_buffer[1024];
        while (fgets(line_buffer, sizeof(line_buffer), file)) {
            size_t line_len = strlen(line_buffer);
            memcpy(input + bytes_read, line_buffer, line_len);
            bytes_read += line_len;
        }
    }

    fclose(file);

    fprintf(stderr, "Bytes read: %zu\n", bytes_read);

    if (bytes_read == 0) {
        fprintf(stderr, "Failed to read any content from the file\n");
        free(input);
        return 1;
    }

    input[bytes_read] = '\0';
    input_len = bytes_read;

    while (input_len > 0 && isspace(input[input_len-1])) {
        input[--input_len] = '\0';
    }

    current_token = get_next_token();

    execute();

    cleanup_variables();

    return 0;
}