
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <limits.h>

#define MAX_LINE_LEN 256   // Maximum length of a line in the input file
#define MAX_IDENTIFIERS 50 // Maximum number of unique identifiers (variables/functions) allowed

char *identifiers[MAX_IDENTIFIERS]; // Array to store identifier names (variables/functions)
int identifier_count = 0;           // Count of currently stored identifiers

// Function to clean up by removing the generated C file and executable
void cleanup(const char *c_filename, const char *exec_filename)
{
    if (remove(c_filename) != 0)
        perror("! Error removing C file");

    if (remove(exec_filename) != 0)
        perror("! Error removing executable");
}

// function to check if new identifier added is valid (max 12 chars and no upper case letter)
int is_valid_identifier(const char *id)
{
    int length = strlen(id);

    if (length > 12)
    {
        return 0;
    }

    for (int i = 0; i < length; i++)
    {
        if (id[i] >= 'A' && id[i] <= 'Z')
        {
            return 0;
        }
    }

    return 1;
}

// helpher function to check if identifier already exists inside the array
int identifier_exists(const char *id)
{
    for (int i = 0; i < identifier_count; i++)
    {
        if (strcmp(identifiers[i], id) == 0)
        {
            return 1;
        }
    }
    return 0;
}

// add identifier into identifier array
void add_identifier(const char *id)
{
    if (identifier_count >= MAX_IDENTIFIERS)
    {
        fprintf(stderr, "! Error: Maximum number of identifiers (%d) exceeded.\n", MAX_IDENTIFIERS);
        exit(1);
    }
    if (!identifier_exists(id))
    {
        identifiers[identifier_count++] = strdup(id);
    }
}

// function to translate mini language syntax into C
// first param = .ml file to be translated || second param =  output file for generated C code
void generate_c_code(FILE *ml_file, const char *c_filename)
{
    FILE *c_file = fopen(c_filename, "w");
    if (!c_file)
    {
        fprintf(stderr, "! Error: Could not create %s\n", c_filename);
        exit(1);
    }

    fprintf(c_file, "#include <stdio.h>\n");
    fprintf(c_file, "#include <math.h>\n\n");
    fprintf(c_file, "double arg0 = 0.0, arg1 = 0.0;\n\n");

    char line[MAX_LINE_LEN];
    int in_function = 0; // Track if we are inside a function definition

    // first pass: Parse the .ml file and write global variable declarations and function prototypes
    while (fgets(line, sizeof(line), ml_file))
    {
        // Remove comments and skip empty lines
        char *comment = strchr(line, '#');
        if (comment)
            *comment = '\0';
        if (strlen(line) == 0 || line[0] == '\n')
            continue;

        // Skip lines that start with a tab (not global assignments, might be a function)
        if (line[0] == '\t')
            continue;

        // function declarations
        if (strncmp(line, "function", 8) == 0)
        {
            // Extract and validate the function name
            char *func_name = strtok(line + 9, " ");

            // Validate the function name
            if (!is_valid_identifier(func_name))
            {
                fprintf(stderr, "! Error: Invalid function name: %s\n", func_name);
                printf("@ Max 12 characters and no upper case characters\n");
                exit(1);
            }

            // Add the function name to the list of identifiers
            add_identifier(func_name);
            continue;
        }
        else if (strchr(line, '<') && strchr(line, '-'))
        {
            // Handle global variable assignments (lines with "<-" operator)
            char *assignment_op = strstr(line, "<-");
            if (assignment_op == NULL)
            {
                fprintf(stderr, "! Error: Invalid global variable assignment\n");
                exit(1);
            }

            // Extract and validate the variable name
            char *var_name = strtok(line, "<-");
            // Trim leading/trailing whitespace for the variable name
            while (*var_name == ' ' || *var_name == '\t')
                var_name++;
            char *var_end = var_name + strlen(var_name) - 1;
            while (var_end > var_name && (*var_end == ' ' || *var_end == '\t'))
                var_end--;
            *(var_end + 1) = '\0'; // Null-terminate the variable name

            // Validate the identifier
            if (!is_valid_identifier(var_name))
            {
                fprintf(stderr, "! Error: Invalid identifier: %s\n", var_name);
                printf("@ Max 12 characters and no upper case characters\n");
                exit(1);
            }

            // Add variable name to identifiers list
            add_identifier(var_name);

            // Extract expression after "<-"
            char *expr = assignment_op + 2; // Skip the "<-"
            // Trim leading/trailing whitespace for the expression
            while (*expr == ' ' || *expr == '\t')
                expr++;
            char *expr_end = expr + strlen(expr) - 1;
            while (expr_end > expr && (*expr_end == ' ' || *expr_end == '\t' || *expr_end == '\n'))
                expr_end--;
            *(expr_end + 1) = '\0'; // Null-terminate the expression

            // Write the global variable assignment to the C file
            fprintf(c_file, "double %s = %s;\n", var_name, expr);
        }
    }
    fprintf(c_file, "\n");

    // reset file pointer for second pass to process function definitions
    fseek(ml_file, 0, SEEK_SET);

    // Second pass: Write function definitions and main
    while (fgets(line, sizeof(line), ml_file))
    {
        // Remove comments and skip empty lines
        char *comment = strchr(line, '#');
        if (comment)
            *comment = '\0';
        if (strlen(line) == 0 || line[0] == '\n')
            continue;

        // Check indentation
        int indentation = 0;
        while (line[indentation] == '\t')
            indentation++;

        // Handle function definitions
        if (strncmp(line, "function", 8) == 0)
        {
            if (in_function)
            {
                // Close the previous function definition
                fprintf(c_file, "    return 0.0;\n}\n\n");
            }
            char *name = strtok(line + 9, " "); // Function name
            char *params = strtok(NULL, "\n");  // Parameters (if any)

            // Start writing the function definition in C syntax
            fprintf(c_file, "double %s(", name);
            char *param = strtok(params, " ");
            int first = 1;
            while (param != NULL)
            {
                if (!first)
                    fprintf(c_file, ", ");
                fprintf(c_file, "double %s ", param); // Declare parameter

                // Add parameter to identifiers list
                add_identifier(param);

                param = strtok(NULL, " ");
                first = 0;
            }
            fprintf(c_file, ") {\n");
            in_function = 1; // indicator we are inside a function
        }
        // Handle function logic (print, return, and assignments inside functions)
        else if (indentation > 0 && in_function)
        {
            char *trimmed_line = line + indentation;

            // Handle 'print' statements inside functions
            if (strncmp(trimmed_line, "print", 5) == 0 && (trimmed_line[5] == ' ' || trimmed_line[5] == '\t' || trimmed_line[5] == '\0'))
            {
                // Handle 'print' keyword
                char *expr = strchr(trimmed_line, ' ') + 1;

                // Remove newline character if present
                char *newline = strchr(expr, '\n');
                if (newline)
                    *newline = '\0';

                // Trim any leading or trailing whitespace
                while (*expr == ' ' || *expr == '\t')
                    expr++;
                char *end = expr + strlen(expr) - 1;
                while (end > expr && (*end == ' ' || *end == '\t' || *end == '\n'))
                    end--;
                *(end + 1) = '\0';

                fprintf(c_file, "    {\n");
                fprintf(c_file, "        double t_value = %s;\n", expr);
                fprintf(c_file, "        if (t_value == (int)t_value) {\n");
                fprintf(c_file, "            printf(\"%%d\\n\", (int)t_value);\n");
                fprintf(c_file, "        } else {\n");
                fprintf(c_file, "            printf(\"%%.6f\\n\", t_value);\n");
                fprintf(c_file, "        }\n");
                fprintf(c_file, "    }\n");
            }

            // Handle 'return' statements inside functions
            else if (strncmp(trimmed_line, "return", 6) == 0)
            {
                char *expr = strchr(trimmed_line, ' ') + 1;

                char *newline = strchr(expr, '\n');
                if (newline)
                    *newline = '\0';

                while (*expr == ' ' || *expr == '\t')
                    expr++;

                char *end = expr + strlen(expr) - 1;

                while (end > expr && (*end == ' ' || *end == '\t' || *end == '\n'))
                    end--;
                *(end + 1) = '\0'; // Null-terminate variable name

                fprintf(c_file, "    double t_value = %s;\n", expr);
                fprintf(c_file, "    return (t_value == (int)t_value) ? (int)t_value : t_value;\n");
            }

            else if (strstr(line, "<-") != NULL)
            {
                // Handle assignments inside functions (replace "<-" with "=")
                char *assignment_op = strstr(line, "<-");
                if (assignment_op == NULL)
                {
                    fprintf(stderr, "! Error: Invalid assignment inside function\n");
                    exit(1);
                }

                // Extract variable name
                char *var_name = strtok(line, "<-");

                // Trim leading/trailing whitespace for the variable name
                while (*var_name == ' ' || *var_name == '\t')
                    var_name++;

                char *var_end = var_name + strlen(var_name) - 1;

                while (var_end > var_name && (*var_end == ' ' || *var_end == '\t'))
                    var_end--;

                *(var_end + 1) = '\0';

                // Validate the identifier
                if (!is_valid_identifier(var_name))
                {
                    fprintf(stderr, "! Error: Invalid identifier: %s\n", var_name);
                    exit(1);
                }

                // Extract expression after "<-"
                char *expr = assignment_op + 2; // Skip the "<-"

                // Trim leading/trailing whitespace for the expression
                while (*expr == ' ' || *expr == '\t')
                    expr++;

                char *expr_end = expr + strlen(expr) - 1;
                while (expr_end > expr && (*expr_end == ' ' || *expr_end == '\t' || *expr_end == '\n'))
                    expr_end--;

                *(expr_end + 1) = '\0'; // Null-terminate the expression

                // Write the variable assignment to the C file
                fprintf(c_file, "double %s = %s;\n", var_name, expr);
            }

            // Handle function calls inside other functions
            else if (strchr(trimmed_line, '('))
            {
                char *func_call = strtok(trimmed_line, "\n");
                fprintf(c_file, "    %s;\n", func_call);
            }
        }
    }

    if (in_function)
    {
        // Close the last function if still open
        fprintf(c_file, "    return 0.0;\n}\n\n");
    }

    // Write the main function
    fprintf(c_file, "int main(int argc, char *argv[]) {\n");

    // Reset file pointer to beginning
    fseek(ml_file, 0, SEEK_SET);

    // Third pass: Write main function body ,handle print statements and function calls

    while (fgets(line, sizeof(line), ml_file))
    {
        // Remove comments and skip empty lines
        char *comment = strchr(line, '#');
        if (comment)
            *comment = '\0';
        if (strlen(line) == 0 || line[0] == '\n')
            continue;

        // Skip function definitions and indented lines in main
        if (strncmp(line, "function", 8) == 0 || line[0] == '\t')
        {
            continue;
        }

        // Handle print statements and function calls in main
        if (strncmp(line, "print", 5) == 0 && (line[5] == ' ' || line[5] == '\t' || line[5] == '\0'))
        {
            char *expr = strchr(line, ' ') + 1;
            fprintf(c_file, "    {\n");
            fprintf(c_file, "        double value = (double)(%s);\n", expr);
            fprintf(c_file, "        if (value == (int)value) {\n");
            fprintf(c_file, "            printf(\"%%d\\n\", (int)value);\n");
            fprintf(c_file, "        } else {\n");
            fprintf(c_file, "            printf(\"%%.6f\\n\", value);\n");
            fprintf(c_file, "        }\n");
            fprintf(c_file, "    }\n");
        }
        else if (strchr(line, '('))
        {
            // Function call in main
            char *func_call = strtok(line, "\n");
            fprintf(c_file, "    %s;\n", func_call);
        }
    }

    fprintf(c_file, "    return 0;\n}\n");

    fclose(c_file);
}

void compile_and_run(const char *c_filename, char *args[], int argc)
{
    // Create a unique name for the executable using the process ID (pid)
    char exec_filename[64];
    snprintf(exec_filename, sizeof(exec_filename), "ml_executable_%d", getpid());

    // Compile the C file into an executable using 'cc'
    char compile_cmd[128];
    snprintf(compile_cmd, sizeof(compile_cmd), "cc -std=c11 -Wall -Werror -o %s %s -lm", exec_filename, c_filename);

    // Run the compile command
    int compile_status = system(compile_cmd);
    if (compile_status != 0)
    {
        fprintf(stderr, "! Error: Compilation failed.\n");
        return;
    }

    // Check if the executable was created
    struct stat st;
    if (stat(exec_filename, &st) != 0)
    {
        fprintf(stderr, "! Error: Executable not found: %s\n", exec_filename);
        return;
    }

    // Set execute permissions (just in case)
    chmod(exec_filename, 0755);

    // Get the current directory
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL)
    {
        // Prepare to execute the compiled program
        char exec_path[PATH_MAX];
        snprintf(exec_path, sizeof(exec_path), "%s/%s", cwd, exec_filename);

        // Prepare the arguments for executing the program
        char *exec_args[argc];
        exec_args[0] = exec_path; // First argument is the path to the executable
        for (int i = 2; i < argc; i++)
        {
            // Copy additional arguments passed by the user (starting from args[2])
            exec_args[i - 1] = args[i];
        }
        exec_args[argc - 1] = NULL; // Null-terminate the argument list

        // Fork a child process to execute the program
        pid_t pid = fork();
        if (pid == 0)
        { // Child process

            // Child process: execute the compiled program with the arguments
            execvp(exec_path, exec_args);
            perror("! Error: execvp failed"); // If execvp fails, error message is shown
            exit(1);
        }
        else if (pid < 0)
        {
            perror("! Error: fork failed"); // fork fail error message
        }
        else
        {
            int status;
            waitpid(pid, &status, 0); // Parent waits for child to finish
        }
    }
    else
    {
        perror("! Error: getcwd failed"); // Failed to get the current working directory, print an error
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "! Usage: %s <filename.ml> [args...]\n", argv[0]); // instruct user to parse in .ml file
        return 1;
    }

    char *ml_filename = argv[1];
    FILE *ml_file = fopen(ml_filename, "r");
    if (!ml_file)
    {
        fprintf(stderr, "! Error: Could not open %s\n", ml_filename); // return error if having error opening .ml file
        return 1;
    }

    // Create a temporary C file to store the translated code
    char c_filename[64];
    snprintf(c_filename, sizeof(c_filename), "ml-%d.c", getpid()); // name C file as process id
    generate_c_code(ml_file, c_filename);                          // translate mini language into C
    fclose(ml_file);

    // Create a temporary C executable file
    char exec_filename[64];
    snprintf(exec_filename, sizeof(exec_filename), "ml_executable_%d", getpid()); // name excecutable file as process id

    // Compile and execute the C code
    compile_and_run(c_filename, argv, argc);

    // Cleanup: remove the generated files
    cleanup(c_filename, exec_filename);

    return 0;
}