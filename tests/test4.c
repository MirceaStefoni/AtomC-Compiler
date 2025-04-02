int len(char s[]){
	int i;
	i=0;
	while(s[i]) i=i+1; 
	return i;
	}

void main(){
	char greeting[20]; 
	char empty[5];
	int len1;
	int len2;

	greeting[0] = 'H';
	greeting[1] = 'e';
	greeting[2] = 'l';
	greeting[3] = 'l';
	greeting[4] = 'o';
	greeting[5] = 0; 

	empty[0] = 0; 

	len1 = len(greeting);
	puti(len1); 

	len2 = len(empty);
	puti(len2); 
}