//Jefferson Boldrin Cardozo - 7591671

#include <pthread.h>	//Para pthreads. Compilar com "gcc -pthreads -o 'output' 'arquivo.c'"
#include <unistd.h>		//Para sleep()
#include <sys/queue.h>	//Para tad fila

#include <stdio.h>
#include <stdlib.h>

//Structs e Typedefs
	//TAD Queue
	struct Node {
		int item;
		struct Node* next;
	};
	typedef struct Node Node;	

	struct Queue {
		Node* head;
		Node* tail;

		void    (*push)     (struct Queue*, int);
		int     (*pop)      (struct Queue*);
		int     (*peek)     (struct Queue*);
		void    (*display)  (struct Queue*);
		int size;
	};
	typedef struct Queue Queue;

	//Structs de Parada, Ônibus e Passageiro
	struct Stp {
		pthread_t thread;		//A thread processando a parada atual
		
		int carro;				//Valor negativo indica que não há nenhum carro atualmente nesta parada
		int contPassageiros;	//Quantos passageiros estão atualmente na esperando nesta parada
	
		Queue passEsperando;	//Fila de passageiros esperando no ponto de ônibus
	};
	typedef struct Stp Stop;

	struct Car {
		pthread_t thread;

		int movimento;		//Se '1', está em movimento. Se '0', está parado
		int stop;			//Se parado, mostra parada atual. Se em movimento, mostra próxima parada
	
		int contPassageiros;	//Quantos passageiros estão atualmente dentro do onibus		
	};
	typedef struct Car Carro;

	struct Pss {
		pthread_t thread;
		
		int viajando;			//Se '1', passageiro ainda não chegou no destino. Se '0', já chegou
		
		int ptoPartida;		
		int ptoChegada;
	};	
	typedef struct Pss Passageiro;


//Variáveis globais
	int terminou = 0;
	int passAindaViajando;

	Stop 		*stops;
	Carro 		*carros;
	Passageiro 	*passageiros;

	pthread_t renderer; 	//Thread que cuida da animação

	int S, C, P, A; //S = bus stops, C = carros, P = passageiros, A = assentos

//Declaração de funções
	void Entrada();			//Recebe e trata a entrada das variáveis S, C, P e A
	void Inicializacao();	//Inicializa as structs e a thread de cada membro
	
	void *Animacao(void *argAnimacao);			//Função da thread de animação
	void *funcStop(void *argStop);				//Função das threads de Paradas
	void *funcCarro(void *argCarro);			//Função das threads de Carros
	void *funcPassageiro(void *argPassageiro);	//Função das threads de Passageiros

	//TAD Queue
	Queue createQueue ();
	void push(Queue* queue, int item);
	int pop(Queue* queue);
	int peek(Queue* queue);
	void display(Queue* queue);

//Main
	int main (){
		
		Entrada();
		Inicializacao();

		while (terminou != 1)
			terminou *= -1;

		return 0;
	}

//Funções Sequenciais
	void Entrada(){
		printf("Numero de paradas (S): ");
		scanf("%d", &S);
		while (S <= 0){
			printf("S deve ser um numero positivo e maior que zero, digite novamente: ");
			scanf("%d", &S);
		}

		printf("Numero de carros (C): ");
		scanf("%d", &C);
			while (C <= 0 || C > S){
			printf("C deve ser um numero positivo, maior que zero e menor ou igual a S, digite novamente: ");
			scanf("%d", &C);
		}

		printf("Numero de passageiros (P): ");
		scanf("%d", &P);
		while (P <= 0 || P <= C){
			printf("P deve ser um numero positivo, maior que zero e maior ou igual C, digite novamente: ");
			scanf("%d", &P);
		}

		printf("Numero de assentos (A): ");
		scanf("%d", &A);
			while (A <= 0 || A <= C || A >= P){
			printf("A deve ser um numero positivo, maior que zero, maior ou igual C e menor ou igual a P, digite novamente: ");
			scanf("%d", &A);
		}
		
		passAindaViajando = P;
	}

	void Inicializacao(){
	//Inicializa arrays das structs Stop, Carro, Passageiro
		stops 		= malloc(sizeof(Stop) 		* S);
		carros 		= malloc(sizeof(Carro) 		* C);
		passageiros = malloc(sizeof(Passageiro) * P);

		int i;
		//Inicializa Stops
		for (i = 0; i < S; ++i){
			stops[i].carro = -1; 			//Valor neg indica nenhum carro
			stops[i].contPassageiros = 0;
			stops[i].passEsperando = createQueue();
		}
		//Inicialize Passageiros
		srand(time(NULL));
		for (i = 0; i < P; ++i){
			passageiros[i].viajando = 1;

			passageiros[i].ptoPartida = rand() % (P-1);
			passageiros[i].ptoChegada = rand() % (P-1);

			if (passageiros[i].ptoPartida == passageiros[i].ptoChegada)
				passageiros[i].ptoChegada++;
		}
		//Inicializa Carros
		for (i = 0; i < C; ++i){
			carros[i].movimento = 0;
			carros[i].stop = rand() % (S-1);

			while (stops[carros[i].stop].carro != -1) {
				carros[i].stop++;
				if (carros[i].stop >= S) carros[i].stop = 0;
			}
			stops[carros[i].stop].carro = i;
		}

	//Inicializa threads
		//Thread de Animação
		if(pthread_create(&renderer, NULL, Animacao, NULL)) {
			fprintf(stderr, "Erro ao criar a thread de animacao, interrompendo programa.\n");
			terminou = 1;
			return;
		}
		//Threads de Paradas de Onibus
		for (i = 0; i < S; ++i){
			if(pthread_create(&stops[i].thread, NULL, funcStop, NULL)) {
				fprintf(stderr, "Erro ao criar uma thread de parada de onibus, interrompendo programa.\n");
				terminou = 1;
				return;
			}
		}
		//Threads de Passageiros
		for (i = 0; i < P; ++i){
			if(pthread_create(&passageiros[i].thread, NULL, funcPassageiro, NULL)) {
				fprintf(stderr, "Erro ao criar uma thread de passageiro, interrompendo programa.\n");
				terminou = 1;
				return;
			}
		}		
		//Threads de Carros
		for (i = 0; i < C; ++i){
			if(pthread_create(&carros[i].thread, NULL, funcCarro, NULL)) {
				fprintf(stderr, "Erro ao criar uma thread de parada de carro, interrompendo programa.\n");
				terminou = 1;
				return;
			}
		}
	}

//Funções paralelas
	void *Animacao(void *argAnimacao){
		while (terminou != 1){
			//printf("\033[H\033[J");

			int i;
			for (i = 0; i < S; i++){
				if (stops[i].carro >= 0) printf("(Carro %d) - ", stops[i].carro);
				else printf("(S/ Carro) - ");
				printf("Parada %d (%d passageiros)\n", (i+1), stops[i].contPassageiros);
			}
			printf("\n");

			for (i = 0; i < C; i++){
				if (carros[i].movimento == 1) printf("Carro %d (prox S: %d) (Ps a bordo: %d)", (i+1), carros[i].stop, carros[i].contPassageiros);
			}

			sleep(2);
		}
	}

	void *funcStop(void *argStop){
		printf("Stop Thread\n");
	}

	void *funcCarro(void *argCarro){
		
	}

	void *funcPassageiro(void *argPassageiro){
		printf("Pass Thread\n");
	}

//Funções do TAD Queue
	void push (Queue* queue, int item) {
		Node* n = (Node*) malloc (sizeof(Node));
		n->item = item;
		n->next = NULL;

		if (queue->head == NULL) {
			queue->head = n;
		}
		else{
			queue->tail->next = n;
		}
		queue->tail = n;
		queue->size++;
	}

	int pop (Queue* queue) {
		Node* head = queue->head;
		int item = head->item;
		queue->head = head->next;
		queue->size--;
		free(head);
		return item;
	}

	int peek (Queue* queue) {
		Node* head = queue->head;
		return head->item;
	}

	void display (Queue* queue) {
		printf("\nDisplay: ");
		if (queue->size == 0) printf("No item in queue.\n");
		else {
			Node* head = queue->head;
			int i, size = queue->size;
			printf("%d item(s):\n", queue->size);
			for (i = 0; i < size; i++) {
				if (i > 0) printf(", ");
				printf("%d", head->item);
				head = head->next;
			}
		}
		printf("\n\n");
	}

	Queue createQueue () {
		Queue queue;
		queue.size = 0;
		queue.head = NULL;
		queue.tail = NULL;
		queue.push = &push;
		queue.pop = &pop;
		queue.peek = &peek;
		queue.display = &display;
		return queue;
}