#include "model.h"
#include "interface.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

// Define the number of rows and columns in the spreadsheet
#define ROWS 10
#define COLS 10

// Enumeration to represent the different types of cell values
typedef enum {
    TEXT,
    NUMBER,
    FORMULA
} valueType;

// Structure to represent a node in the dependency linked list
typedef struct dependencyNode {
    ROW row;
    COL col;
    struct dependencyNode* next;
} dependencyNode;

// Structure to represent a cell in the spreadsheet
typedef struct {
    char *value;
    char *computed_value;
    valueType type;
    char *formula;
    dependencyNode *dependencies;
} cell;

// Double pointer to represent the spreadsheet
cell **spreadsheet;

// Function to check if a given text is a number
int is_number(char *text) {
    char *end;
    strtod(text, &end);
    return end != text && *end == '\0';
}

// Function to evaluate a formula and update dependencies
double evaluate_formula(char *formula, ROW drow, COL dcol) {
    double result = 0;
    char *formula_copy = strdup(formula);  // Create a copy of the formula.
    char *component = strtok(formula_copy + 1, "+");  // Skip the '=' at the start of the formula.

    // Loop through each component of the formula
    while (component != NULL) {
        if (!is_number(component)) {
            // The component is not a number, process it as a cell reference.
            ROW row = component[1] - '1';  // Convert from '1'-'9' to 0-8.
            COL col = toupper(component[0]) - 'A';  // Convert from 'A'-'Z' to 0-25.

            if (spreadsheet[row][col].type == NUMBER) {
                result += strtod(spreadsheet[row][col].value, NULL);
            }
            else if (spreadsheet[row][col].type == FORMULA) {
                result += evaluate_formula(spreadsheet[row][col].formula, row, col);
            }

            dependencyNode *current = spreadsheet[row][col].dependencies;
            while (current != NULL) {
                if (current->row == row && current->col == col) {
                    // Dependency already exists, no need to add a new one
                    break;
                }
                current = current->next;
            }

            // If the dependency doesn't exist, add a new one
            if (current == NULL) {
                dependencyNode *newNode = malloc(sizeof(dependencyNode));
                newNode->row = drow;
                newNode->col = dcol;
                newNode->next = spreadsheet[row][col].dependencies;
                spreadsheet[row][col].dependencies = newNode;
            }
        }
        else {
            // The component is a number, add it to the result
            result += strtod(component, NULL);
        }
        component = strtok(NULL, "+");
    }
    free(formula_copy);  // Free the copied string.
    return result;
}

// Function to initialize the model and the spreadsheet
void model_init() {
    spreadsheet = malloc(ROWS * sizeof(cell *));
    for (int i = 0; i < ROWS; i++) {
        spreadsheet[i] = malloc(COLS * sizeof(cell));
        for (int j = 0; j < COLS; j++) {
            spreadsheet[i][j].value = NULL;
            spreadsheet[i][j].type = TEXT;
            spreadsheet[i][j].dependencies = NULL;
        }
    }
}

// Function to update dependents of a cell
void update_dependents(ROW row, COL col) {
    dependencyNode *current = spreadsheet[row][col].dependencies;
    while (current != NULL) {
        if (spreadsheet[current->row][current->col].type == FORMULA) {
            // The dependent cell is a formula cell, update its computed value.
            double result = evaluate_formula(spreadsheet[current->row][current->col].formula, current->row, current->col);
            if (spreadsheet[current->row][current->col].computed_value != NULL) {
                free(spreadsheet[current->row][current->col].computed_value);
            }
            char *result_str = malloc(20 * sizeof(char));  // Allocate enough space for a double.
            sprintf(result_str, "%.1f", result);
            spreadsheet[current->row][current->col].computed_value = result_str;
            update_cell_display(current->row, current->col, spreadsheet[current->row][current->col].computed_value);
        }
        current = current->next;
    }
}

// Function to set the value of a cell
void set_cell_value(ROW row, COL col, char *text) {
    // Free existing values in the cell
    if (spreadsheet[row][col].value != NULL) {
        free(spreadsheet[row][col].value);
    }
    if (spreadsheet[row][col].computed_value != NULL) {
        free(spreadsheet[row][col].computed_value);
    }

    // Check if the input text is a formula or a number
    if (text[0] == '=') {
        // The input text is a formula
        spreadsheet[row][col].type = FORMULA;
        spreadsheet[row][col].formula = strdup(text);
        double result = evaluate_formula(text, row, col);
        char *result_str = malloc(20 * sizeof(char));  // Allocate enough space for a double.
        sprintf(result_str, "%.1f", result);
        spreadsheet[row][col].value = strdup(text);
        spreadsheet[row][col].computed_value = result_str;
    } else {
        // The input text is a number or text
        if (is_number(text)) {
            spreadsheet[row][col].type = NUMBER;
            char *value_str = malloc(20 * sizeof(char));  // Allocate enough space for a double.
            sprintf(value_str, "%.1f", strtod(text, NULL));
            spreadsheet[row][col].value = value_str;
            spreadsheet[row][col].computed_value = strdup(value_str);
        } else {
            spreadsheet[row][col].type = TEXT;
            spreadsheet[row][col].value = strdup(text);
            spreadsheet[row][col].computed_value = strdup(text);
        }
        spreadsheet[row][col].formula = NULL;
    }

    // Update the display and dependents
    update_cell_display(row, col, spreadsheet[row][col].computed_value);
    update_dependents(row, col);

    // Free the input text
    free(text);
}

// Function to clear the value of a cell
void clear_cell(ROW row, COL col) {
    // Free existing values in the cell
    if (spreadsheet[row][col].value != NULL) {
        free(spreadsheet[row][col].value);
        spreadsheet[row][col].value = NULL;
    }
    if (spreadsheet[row][col].formula != NULL) {
        free(spreadsheet[row][col].formula);
        spreadsheet[row][col].formula = NULL;
    }

    // Set the cell type to TEXT and update the display
    spreadsheet[row][col].type = TEXT;
    update_cell_display(row, col, "");
}

// Function to get the textual value of a cell
char *get_textual_value(ROW row, COL col) {
    // Check if the cell contains a formula or a number
    if (spreadsheet[row][col].type == FORMULA) {
        // Return the formula text
        return spreadsheet[row][col].value != NULL ? strdup(spreadsheet[row][col].value) : strdup("");
    } else {
        // Return the computed value text
        return spreadsheet[row][col].computed_value != NULL ? strdup(spreadsheet[row][col].computed_value) : strdup("");
    }
}
