#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/*
  TODO
  -*- mode: compilation; default-directory: "~/xos/projects/c/lisp/" -*-
  Compilation started at Sun Aug  4 02:48:36
  
  gcc main.c && ./a.out src.lisp 
  => 4
  => 4
  => (+ 3 1)
  => (a b c)
  Unexpected ')'

  Compilation exited abnormally with code 1 at Sun Aug  4 02:48:36, duration 0.06 s
*/


// Define basic data types for the interpreter
typedef enum { INT, SYMBOL, CONS, NIL } Type;

typedef struct Object {
    Type type;
    union {
        int value;
        char *name;
        struct {
            struct Object *car;
            struct Object *cdr;
        };
    };
} Object;

Object *nil;

Object *make_int(int value) {
    Object *obj = malloc(sizeof(Object));
    obj->type = INT;
    obj->value = value;
    return obj;
}

Object *make_symbol(char *name) {
    Object *obj = malloc(sizeof(Object));
    obj->type = SYMBOL;
    obj->name = strdup(name);
    return obj;
}

Object *make_cons(Object *car, Object *cdr) {
    Object *obj = malloc(sizeof(Object));
    obj->type = CONS;
    obj->car = car;
    obj->cdr = cdr;
    return obj;
}

// Basic environment for symbols
typedef struct Env {
    char *name;
    Object *value;
    struct Env *next;
} Env;

Env *global_env = NULL;

Object *env_get(Env *env, char *name) {
    while (env) {
        if (strcmp(env->name, name) == 0) {
            return env->value;
        }
        env = env->next;
    }
    return NULL;
}

void env_set(Env **env, char *name, Object *value) {
    Env *e = malloc(sizeof(Env));
    e->name = strdup(name);
    e->value = value;
    e->next = *env;
    *env = e;
}

// Tokenizer
char *token;
char *next_token(char **input) {
    while (**input && isspace(**input)) (*input)++;
    if (**input == '\0') return NULL;

    char *start = *input;

    if (**input == '(' || **input == ')' || **input == '\'') {
        (*input)++;
    } else {
        while (**input && !isspace(**input) && **input != '(' && **input != ')' && **input != '\'') {
            (*input)++;
        }
    }

    size_t len = *input - start;
    token = malloc(len + 1);
    strncpy(token, start, len);
    token[len] = '\0';
    return token;
}

// Parser
Object *parse_expr(char **input);

Object *parse_list(char **input) {
    Object *car = NULL;
    Object *cdr = NULL;
    Object *head = NULL;
    Object *tail = NULL;

    while (**input && **input != ')') {
        car = parse_expr(input);
        if (car == NULL) break;

        cdr = nil;

        Object *node = make_cons(car, cdr);
        if (head == NULL) {
            head = node;
            tail = node;
        } else {
            tail->cdr = node;
            tail = node;
        }
    }

    if (**input == ')') {
        next_token(input); // consume ')'
    } else {
        fprintf(stderr, "Expected ')'\n");
        exit(1);
    }

    return head;
}

Object *parse_expr(char **input) {
    next_token(input);
    if (strcmp(token, "(") == 0) {
        return parse_list(input);
    } else if (strcmp(token, ")") == 0) {
        fprintf(stderr, "Unexpected ')'\n");
        exit(1);
    } else if (strcmp(token, "'") == 0) {
        // Handle quoting with '
        Object *quoted_expr = parse_expr(input);
        Object *quote_symbol = make_symbol("quote");
        return make_cons(quote_symbol, make_cons(quoted_expr, nil));
    } else if (isdigit(token[0]) || (token[0] == '-' && isdigit(token[1]))) {
        return make_int(atoi(token));
    } else {
        return make_symbol(token);
    }
}

// Evaluator
Object *eval(Object *expr, Env *env);

Object *eval_list(Object *list, Env *env) {
    if (list == nil) return nil;
    Object *car = eval(list->car, env);
    Object *cdr = eval_list(list->cdr, env);
    return make_cons(car, cdr);
}

Object *apply(Object *fn, Object *args, Env *env) {
    if (fn->type != SYMBOL) {
        fprintf(stderr, "Attempt to call a non-symbol\n");
        exit(1);
    }

    if (strcmp(fn->name, "+") == 0) {
        int sum = 0;
        while (args != nil) {
            sum += eval(args->car, env)->value;
            args = args->cdr;
        }
        return make_int(sum);
    } else if (strcmp(fn->name, "-") == 0) {
        if (args == nil) return make_int(0);
        int diff = eval(args->car, env)->value;
        args = args->cdr;
        while (args != nil) {
            diff -= eval(args->car, env)->value;
            args = args->cdr;
        }
        return make_int(diff);
    } else if (strcmp(fn->name, "*") == 0) {
        int prod = 1;
        while (args != nil) {
            prod *= eval(args->car, env)->value;
            args = args->cdr;
        }
        return make_int(prod);
    } else if (strcmp(fn->name, "/") == 0) {
        if (args == nil) {
            fprintf(stderr, "Division by zero\n");
            exit(1);
        }
        int quotient = eval(args->car, env)->value;
        args = args->cdr;
        while (args != nil) {
            int divisor = eval(args->car, env)->value;
            if (divisor == 0) {
                fprintf(stderr, "Division by zero\n");
                exit(1);
            }
            quotient /= divisor;
            args = args->cdr;
        }
        return make_int(quotient);
    } else if (strcmp(fn->name, "quote") == 0) {
        return args->car;
    } else {
        fprintf(stderr, "Unknown function '%s'\n", fn->name);
        exit(1);
    }
}

Object *eval(Object *expr, Env *env) {
    switch (expr->type) {
    case INT:
        return expr;
    case SYMBOL: {
        Object *value = env_get(env, expr->name);
        if (!value) {
            fprintf(stderr, "Unbound symbol '%s'\n", expr->name);
            exit(1);
        }
        return value;
    }
    case CONS: {
        Object *fn = expr->car;
        Object *args = expr->cdr;
        return apply(fn, args, env);
    }
    case NIL:
        return nil;
    }
    return nil;
}

void print_object(Object *obj) {
    switch (obj->type) {
    case INT:
        printf("%d", obj->value);
        break;
    case SYMBOL:
        printf("%s", obj->name);
        break;
    case CONS:
        printf("(");
        print_object(obj->car);
        Object *cdr = obj->cdr;
        while (cdr != nil && cdr->type == CONS) {
            printf(" ");
            print_object(cdr->car);
            cdr = cdr->cdr;
        }
        if (cdr != nil) {
            printf(" . ");
            print_object(cdr);
        }
        printf(")");
        break;
    case NIL:
        printf("nil");
        break;
    }
}

void repl(FILE *input) {
    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), input)) {
        // Skip comments
        if (buffer[0] == ';') continue;

        char *input_ptr = buffer;
        Object *expr = parse_expr(&input_ptr);
        Object *result = eval(expr, global_env);
        printf("=> ");
        print_object(result);
        printf("\n");
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file.lisp>\n", argv[0]);
        return 1;
    }

    nil = malloc(sizeof(Object));
    nil->type = NIL;

    // Initialize the global environment with built-in functions
    env_set(&global_env, "+", make_symbol("+"));
    env_set(&global_env, "-", make_symbol("-"));
    env_set(&global_env, "*", make_symbol("*"));
    env_set(&global_env, "/", make_symbol("/"));
    env_set(&global_env, "quote", make_symbol("quote"));

    FILE *file = fopen(argv[1], "r");
    if (!file) {
        perror("Error opening file");
        return 1;
    }

    repl(file);

    fclose(file);
    return 0;
}
