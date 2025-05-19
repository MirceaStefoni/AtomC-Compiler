//Atribuire la o constantă: 5 = text[i];
//Verificare if cu conditie non-scalara: if(v)
//Apel de funcție cu prea puține argumente: f(v[x].text, y);
//Apel de funcție cu prea multe argumente: f(v[x].text, y, '#', 1);
//Indexare a unui non-array: ch = ch[i];
//Returnare incorectă într-o funcție void: return 5;
//Tipuri incompatibile în apelul de funcție: f(42, 'a', 3.14);
//Accesarea unui membru inexistent al unei structuri: v[x].nonexistent
//Tipuri incompatibile pentru operatori: x+v[x]
//Apelarea unei non-funcții: a()
//Utilizarea unei funcții fără apel: h;
//Tipuri incompatibile în return int: return v[x];
//Index de tip incompatibil: v["string"]
//Atribuire directă la un vector: v=5;
//Accesarea unui membru ca la o structură: array[1].text;
//Apelarea unei funcții nedefinite: g();

struct S{
	int n;
	char text[16];
	};
	
struct S a;
struct S v[10];

void f(char text[],int i,char ch){
	text[i]=ch;
	}

int h(int x,int y){
	if(x>0&&x<y){
		f(v[x].text,y,'#');
		return 1;
		}
	return 0;
	}
