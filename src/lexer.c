#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "lexer.h"
#include "utils.h"

Token *tokens; // single linked list of tokens
Token *lastTk; // the last token in list

int line = 1; // the current line in the input file

// adds a token to the end of the tokens list and returns it
// sets its code and line
Token *addTk(int code)
{
	Token *tk = safeAlloc(sizeof(Token));
	tk->code = code;
	tk->line = line;
	tk->next = NULL;
	if (lastTk)
	{
		lastTk->next = tk;
	}
	else
	{
		tokens = tk;
	}
	lastTk = tk;
	return tk;
}

char *extract(const char *begin, const char *end)
{
	char *text = safeAlloc(end - begin + 1);
	memcpy(text, begin, end - begin);
	text[end - begin] = '\0';
	return text;
}

Token *tokenize(const char *pch)
{

	const char *start;
	Token *tk;
	for (;;)
	{
		switch (*pch)
		{
		case ' ':
		case '\t':
			pch++;
			break;
		case '\r': // handles different kinds of newlines (Windows: \r\n, Linux: \n, MacOS, OS X: \r or \n)
			if (pch[1] == '\n')
				pch++;
			// fallthrough to \n

			// Delimitatori

		case '\n':
			line++;
			pch++;
			break;

		case '\0':
			addTk(END);
			return tokens;

		case ',':
			addTk(COMMA);
			pch++;
			break;

		case ';':
			addTk(SEMICOLON);
			pch++;
			break;

		case '(':
			addTk(LPAR);
			pch++;
			break;

		case ')':
			addTk(RPAR);
			pch++;
			break;

		case '[':
			addTk(LBRACKET);
			pch++;
			break;

		case ']':
			addTk(RBRACKET);
			pch++;
			break;

		case '{':
			addTk(LACC);
			pch++;
			break;

		case '}':
			addTk(RACC);
			pch++;
			break;

			// Operatori

		case '=':
			if (pch[1] == '=')
			{
				addTk(EQUAL);
				pch += 2;
			}
			else
			{
				addTk(ASSIGN);
				pch++;
			}
			break;

		case '+':
			addTk(ADD);
			pch++;
			break;

		case '-':
			addTk(SUB);
			pch++;
			break;

		case '*':
			addTk(MUL);
			pch++;
			break;

		case '/':
			if (pch[1] == '/') {  // Comentariu single-line
				pch += 2;
				while (*pch && *pch != '\n') {  // Continua pana la endl sau EOF
					pch++;
				}
				if (*pch == '\n') {
					line++;
					pch++;
				}
			} else {
				addTk(DIV);
				pch++;
			}
			break;

		case '.':
			addTk(DOT);
			pch++;
			break;

		case '&':
			if (pch[1] == '&')
			{
				addTk(AND);
				pch += 2;
			}
			else
			{
				err("invalid char: %c (%d)", *pch, *pch);
				pch++;
			}
			break;

		case '|':
			if (pch[1] == '|')
			{
				addTk(OR);
				pch += 2;
			}
			else
			{
				err("invalid char: %c (%d)", *pch, *pch);
				pch++;
			}
			break;

		case '!':
			if (pch[1] == '=')
			{
				addTk(NOTEQ);
				pch += 2;
			}
			else
			{
				addTk(NOT);
				pch++;
			}
			break;

		case '<':
			if (pch[1] == '=')
			{
				addTk(LESSEQ);
				pch += 2;
			}
			else
			{
				addTk(LESS);
				pch++;
			}
			break;

		case '>':
			if (pch[1] == '=')
			{
				addTk(GREATEREQ);
				pch += 2;
			}
			else
			{
				addTk(GREATER);
				pch++;
			}
			break;

			// Definirea tipurilor de date

		default:
			// 1. Identificatori și cuvinte cheie
			if (isalpha(*pch) || *pch == '_')
			{
				start = pch++;
				while (isalnum(*pch) || *pch == '_')
					pch++;

				char *text = extract(start, pch);

				// Verificare cuvinte cheie
				struct
				{
					const char *keyword;
					int token;
				} keywords[] = {
					{"char", TYPE_CHAR},
					{"int", TYPE_INT},
					{"double", TYPE_DOUBLE},
					{"void", VOID},
					{"struct", STRUCT},
					{"if", IF},
					{"else", ELSE},
					{"while", WHILE},
					{"return", RETURN},
					{NULL, 0}};

				int i;
				for (i = 0; keywords[i].keyword != NULL; i++)
				{
					if (strcmp(text, keywords[i].keyword) == 0)
					{
						addTk(keywords[i].token);
						break;
					}
				}

				if (keywords[i].keyword == NULL)
				{
					tk = addTk(ID);
					tk->text = text;
				}
			}

			// 2. Numere intregi si reale
			else if (isdigit(*pch) || (*pch == '.' && isdigit(pch[1])))
			{
				start = pch;
				int isDouble = 0;

				while (isdigit(*pch))
					pch++; // partea intreaga
				if (*pch == '.')
				{ // parte zecimala
					isDouble = 1;
					pch++;
					while (isdigit(*pch))
						pch++;
				}
				if (*pch == 'e' || *pch == 'E')
				{ // notatie
					isDouble = 1;
					pch++;
					if (*pch == '+' || *pch == '-')
						pch++;
					if (!isdigit(*pch))
						err("Notatie stintifica invalida!!");
					while (isdigit(*pch))
						pch++;
				}

				
				if (isalpha(*pch) || *pch == '_') {
					err("Format invalid de numar: nu poate contine litere sau underscore");
				}

				char *text = extract(start, pch);
				tk = addTk(isDouble ? DOUBLE : INT);
				if (isDouble) {
					tk->d = atof(text);
				} else {
					tk->i = atoi(text);
				}
				free(text);
			}

			// 3. Caractere
			else if (*pch == '\'')
			{
				pch++;
				tk = addTk(CHAR);

				if (*pch == '\\')
				{
					pch++;
					tk->c = *pch;
				}
				else
				{
					tk->c = *pch;
				}

				pch++;
				if (*pch != '\'')
					err("Lipeste ghilimeaua de inchidere");
				pch++;
			}

			// 4. Siruri de caractere
			else if (*pch == '"')
			{
				start = ++pch;
				while (*pch != '"')
				{
					if (*pch == '\0')
						err("Sir neterminat");
					if (*pch == '\\')
					{
						pch += 2;
					}
					else
					{
						pch++;
					}
				}
				tk = addTk(STRING);
				tk->text = extract(start, pch);
				pch++;
			}

			// 5. Caracter invalid
			else
			{
				err("Caracter invalid: %c (%d)", *pch, *pch);
			}
		}
	}
}

const char* getTokenName(int code) {
    switch(code) {
        case ID: return "ID";
        case TYPE_CHAR: return "TYPE_CHAR";
        case TYPE_DOUBLE: return "TYPE_DOUBLE";
        case ELSE: return "ELSE";
        case IF: return "IF";
        case TYPE_INT: return "TYPE_INT";
        case RETURN: return "RETURN";
        case STRUCT: return "STRUCT";
        case VOID: return "VOID";
        case WHILE: return "WHILE";
        case COMMA: return "COMMA";
        case SEMICOLON: return "SEMICOLON";
        case LPAR: return "LPAR";
        case RPAR: return "RPAR";
        case LBRACKET: return "LBRACKET";
        case RBRACKET: return "RBRACKET";
        case LACC: return "LACC";
        case RACC: return "RACC";
        case END: return "END";
        case ASSIGN: return "ASSIGN";
        case EQUAL: return "EQUAL";
        case ADD: return "ADD";
        case SUB: return "SUB";
        case MUL: return "MUL";
        case DIV: return "DIV";
        case DOT: return "DOT";
        case AND: return "AND";
        case OR: return "OR";
        case NOT: return "NOT";
        case NOTEQ: return "NOTEQ";
        case LESS: return "LESS";
        case LESSEQ: return "LESSEQ";
        case GREATER: return "GREATER";
        case GREATEREQ: return "GREATEREQ";
        case STRING: return "STRING";
        case INT: return "INT";
        case CHAR: return "CHAR";
        case DOUBLE: return "DOUBLE";
        default: return "UNKNOWN";
    }
}

void showTokens(const Token *tokens)
{

	FILE *fp = fopen("results/tokens.txt", "w");
    if (fp == NULL) {
        perror("Error opening results/tokens.txt");
        return;
    }

	for (const Token *tk = tokens; tk; tk = tk->next)
	{
		fprintf(fp, "%d\t%s", tk->line, getTokenName(tk->code));
		
		switch(tk->code) {
			case ID:
			case STRING:
				fprintf(fp, ":%s", tk->text);
				break;
			case INT:
				fprintf(fp, ":%d", tk->i);
				break;
			case CHAR:
				fprintf(fp, ":%c", tk->c);
				break;
			case DOUBLE:
				fprintf(fp, ":%g", tk->d);
				break;
		}
		fprintf(fp, "\n");
	}
	fclose(fp);
}
