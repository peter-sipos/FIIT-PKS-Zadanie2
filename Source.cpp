#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma comment(lib, "ws2_32.lib") // sluzi na pridanie winsocku do dependencies linkeru visualka

//#include <Winsock2.h>
#include <WS2tcpip.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

// prevzata funkcia na vypocet crc32. zdroj ---> https://stackoverflow.com/questions/21001659/crc32-algorithm-implementation-in-c-without-a-look-up-table-and-with-a-public-li
unsigned int crc32b(unsigned char *message) {
	int i, j;
	unsigned int byte, crc, mask;

	i = 0;
	crc = 0xFFFFFFFF;
	while (message[i] != 0) {
		byte = message[i];            // Get next byte.
		crc = crc ^ byte;
		for (j = 7; j >= 0; j--) {    // Do eight times.
#pragma warning( disable : 4146 )
			mask = -(crc & 1);
#pragma warning( default : 4146 )
			crc = (crc >> 1) ^ (0xEDB88320 & mask);
		}
		i = i + 1;
	}
	return ~crc;
}

typedef struct normal_header {
	uint8_t TYPE;
	uint32_t FRAG_AMOUNT;
	uint32_t FRAG_NUMBER;
	uint32_t FRAG_SIZE;
	uint32_t CHECKSUM;
};


typedef struct keep_alive_header {
	uint8_t TYPE;
	uint8_t OPERATION;
};

typedef struct received_header {
	uint8_t TYPE;
	uint8_t CONFIRMATION;
	uint32_t FRAG_NUMBER;
};

typedef struct data_to_send {
	struct normal_header header;
	char message[1500];
};


void sender();
void receiver();
void receive_text();
void receive_file();
void send_text(char* ip_address, int port, int frag_size);
void send_corrupted(char* ip_address, int port, int frag_size);
void send_file(char* ip_address, int port, int frag_size);
struct data_to_send request_fragment(int frag_number, SOCKET socket_file_descriptor,
	struct sockaddr_in sender_addr,
	int sender_size, uint32_t *checksum_original, uint32_t *checksum_received);


int main() {
	char mode;

	while (1) {
		printf("Akym sposobom chcete spustit program? Ako prijimatela, alebo odosielatela? \n"
			"Pre prijimatela zadajte \"p\", pre odosielatela zasa \"o\". Ak si prajete program ukoncit, zadajte \"k\". \n");
		scanf("%c", &mode);
		getchar();
		if (mode == 'o') {
			//printf("spusti sendera\n");
			sender();
			getchar();
			continue;
		}
		else if (mode == 'p') {
			//printf("spusti prijemcu\n");
			receiver();
			getchar();
			continue;
		}
		else if (mode == 'k')
			break;
		else
			printf("Zadali ste nevalidny mod. \n"
				"Zadajte prosim \"o\" pre spustenie odosielatela, \"p\" pre prijmatela alebo \"k\" pre ukoncenie programu.\n");

	}

	getchar(); getchar(); // VYMAZAT, CISTO KVOLI DEVELOPMENTU
	return 0;
}


void sender() {
	char IP_adress_string[20], operation[10], stay[5];
	unsigned long IP_adress;
	short port;
	uint32_t frag_size;


	printf("Zadajte prosim cielovu IP adresu, port a maximalnu velkost fragmentov.\n"
		"Udaje zadajte v tomto poradi a oddelene medzerou.\n"
		"IP adresu zadajte v decimalnej bodkovej notacii (cislo.cislo.cislo.cilo).\n"
		"Minimalna velkost fragmentu je 1B, maximalna je 1457B. Zadajte prosim iba cislo z tohto rozsahu.\n");
	scanf("%s %hd %u", IP_adress_string, &port, &frag_size);
	while (frag_size == 0 || frag_size > 1457) {
		printf("Zadali ste velkost fragmentu mimo povoleneho rozsahu. Povoleny rozsah je 1B az 1457B\n");
		scanf("%u", &frag_size);
	}

	while (1) {

		printf("Chcete odoslat textovu spravu, chybny fragment alebo textovy subor?\n"
			"Pre odoslanie spravy zadajte \"text\", pre skusku odoslania chybneho fragmentu\n"
			"zadajte \"chybny\" a pre odoslanie suboru zadajte \"subor\".\n");
		scanf("%s", operation);
		while (1) {
			if (strcmp(operation, "text") == 0) {
				send_text(IP_adress_string, port, frag_size);
			}
			else if (strcmp(operation, "chybny") == 0) {
				send_corrupted(IP_adress_string, port, frag_size);
			}
			else if (strcmp(operation, "subor") == 0) {
				send_file(IP_adress_string, port, frag_size);
			}
			else {
				printf("Zadali ste nevalidnu operaciu.Pre odoslanie textovej spravy zadajte \"text\", \n"
					"pre otestovanie zachytenia chybneho fragmentu zadajte \"chybny\" \n"
					"a pre odoslanie textoveho suboru zadajte \"subor\".\n");
				scanf("%s", operation);
				continue;
			}

			printf("Prajete si, aby program zotrval v aktualnom rezime? Ak ano, zadajte \"ano\", v opacnom pripade zadajte \"nie\".\n");
			while (1) {
				scanf("%s", stay);
				if (strcmp(stay, "nie") == 0)
					return;
				else if (strcmp(stay, "ano") == 0)
					break;
				else
					printf("Zadali ste nieco divne. Prosim, zadajte \"ano\" alebo \"nie\".\n");
			}
			break; // aby opat vypisal aj uvodny pokec
		}
	}

}

void receiver() {
	char operation[10], stay[5];

	while (1) {

		printf("Chcete prijat textovu spravu, alebo textovy subor?\n"
			"Pre prijatie spravy zadajte \"text\", pre prijatie suboru zadajte \"subor\".\n");
		scanf("%s", operation);
		while (1) {
			if (strcmp(operation, "text") == 0) {
				receive_text();
			}
			else if (strcmp(operation, "subor") == 0) {
				receive_file();
			}
			else {
				printf("Zadali ste nevalidnu operaciu. Pre prijatie spravy zadajte \"text\", pre prijatie suboru zadajte \"subor\".\n");
				scanf("%s", operation);
				continue;
			}

			printf("Prajete si, aby program zotrval v aktualnom rezime? Ak ano, zadajte \"ano\", v opacnom pripade zadajte \"nie\".\n");
			while (1) {
				scanf("%s", stay);
				if (strcmp(stay, "nie") == 0)
					return;
				else if (strcmp(stay, "ano") == 0)
					break;
				else
					printf("Zadali ste nieco divne. Prosim, zadajte \"ano\" alebo \"nie\".\n");
			}
			break; // aby opat vypisal aj uvodny pokec
		}
	}
}

void receive_text() {
	int frag_amount, frag_number, frag_size, fragments_received,
		message_length, i, position, check, sender_addr_size, sender_size, descriptor_ready;
	char **string_arr;
	struct sockaddr_in sender_addr;
	WSADATA winsock_stuff;
	SOCKET socket_file_descriptor;
	short port;
	struct data_to_send my_data;
	uint32_t checksum_original, checksum_received;
	fd_set file_descriptors, file_descriptors_copy;
	struct timeval timer;

	printf("Zadajte prosim cislo portu.\n");
	scanf("%hd", &port);

	check = WSAStartup(MAKEWORD(2, 2), &winsock_stuff);
	if (check) {
		printf("Bohuzial, nepodarilo sa rozbehat winsock. Tu je cislo daneho erroru: %d.\n"
			"Vela stastia pri jeho rieseni. Kym ho najdete, program sa zatial vypne. Ked budete mat riesenie, nevahajte ho opat spustit.", check);
		exit(EXIT_FAILURE);
	}

	// vytvorenie socketu
	socket_file_descriptor = INVALID_SOCKET;
	socket_file_descriptor = socket(AF_INET, SOCK_DGRAM, 0); 
	if (socket_file_descriptor == INVALID_SOCKET) {
		printf("ERROR: nepodarilo sa vytvorit socket. Program sa nasledne sam vypne. Skuste ho znovu spustit. Error cislo: %d.\n", WSAGetLastError());
		getchar();
		exit(EXIT_FAILURE);
	}

	// vyplnenie adresy
	sender_addr.sin_family = AF_INET;
	sender_addr.sin_addr.S_un.S_addr = INADDR_ANY;
	sender_addr.sin_port = htons(port);

	check = bind(socket_file_descriptor, (SOCKADDR *)&sender_addr, sizeof(sender_addr));
	if (check) {
		printf("Z neznameho dovodu zlyhal bind. Error cislo: %d.\n", WSAGetLastError());
		exit(EXIT_FAILURE);
	}

	printf("Cakam na spravu od prijmatela.\n");

	sender_size = sizeof(sender_addr);
	frag_number = 0;
	frag_amount = 0;
	fragments_received = 0;

	// nastavenie selectu
	FD_ZERO(&file_descriptors);
	FD_SET(socket_file_descriptor, &file_descriptors);
	timer.tv_sec = 10;
	timer.tv_usec = 0;

	while (1) { // prijmam az kym neprijmem nejaky spravny
		file_descriptors_copy = file_descriptors;
		descriptor_ready = select(socket_file_descriptor, &file_descriptors_copy, NULL, NULL, &timer);
		if (descriptor_ready == 0)
		{
			printf("Prvy packet zatial neprisiel, cakam na dalsi.\n");
		}
		else if (descriptor_ready == -1)
		{
			printf("Select vyhodil chybu lebo %d.\n", WSAGetLastError());
			exit(EXIT_FAILURE);
		}
		else {
			check = recvfrom(socket_file_descriptor, (char*)&my_data, sizeof(data_to_send), 0, (SOCKADDR *)&sender_addr, &sender_size);
			if (check == SOCKET_ERROR) {
				printf("Houston, mame problem. Prijem sa nepodaril. Error cislo: %d.\n", WSAGetLastError());
				exit(EXIT_FAILURE);
			}

			checksum_original = my_data.header.CHECKSUM;
			checksum_received = crc32b((unsigned char*)&my_data);
			if (checksum_received == checksum_original)
				break;
			fragments_received++; // pre pripad, ze prvy pride no je poskodeny
		}
	}

	frag_size = my_data.header.FRAG_SIZE;
	frag_amount = my_data.header.FRAG_AMOUNT;
	frag_number = my_data.header.FRAG_NUMBER;
	string_arr = (char**)malloc(frag_amount * sizeof(char*));
	for (i = 0; i < frag_amount; i++)
		string_arr[i] = (char*)malloc(frag_size + 1);
	for (i = 0; i < frag_amount; i++)
		string_arr[i][0] = '\0';

	printf("prijate fragmenty budu mat max velkost %dB\n", frag_size);

	strcpy(string_arr[frag_number], my_data.message);
	fragments_received++;

	timer.tv_sec = 0;
	timer.tv_usec = 100;
	while (fragments_received != frag_amount) {
		file_descriptors_copy = file_descriptors;
		FD_ZERO(&file_descriptors_copy);
		FD_SET(socket_file_descriptor, &file_descriptors_copy);
		descriptor_ready = select(socket_file_descriptor, &file_descriptors_copy, NULL, NULL, &timer);
		if (descriptor_ready == 0)
		{
			fragments_received++; // viem, ze nejaky mal prist, ale neprisiel, tak aj tak navysim pocet prijatych, aby som niekedy ukoncil slucku
		}
		else if (descriptor_ready == -1)
		{
			printf("Select vyhodil chybu cislo: %d.\n", WSAGetLastError());
			exit(EXIT_FAILURE);
		}
		else {
			check = recvfrom(socket_file_descriptor, (char*)&my_data, sizeof(my_data), 0, (SOCKADDR *)&sender_addr, &sender_size);
			if (check == SOCKET_ERROR) {
				printf("Houston, mame problem. Prijem sa nepodaril. Error cislo: %d.\n", WSAGetLastError());
				exit(EXIT_FAILURE);
			}

			checksum_received = crc32b((unsigned char*)&my_data);
			checksum_original = my_data.header.CHECKSUM;
			if (checksum_received != checksum_original) {
				fragments_received++;
				continue;
			}
			frag_number = my_data.header.FRAG_NUMBER;
			strcpy(string_arr[frag_number], my_data.message);
			fragments_received++;
		}
	}

	// znova si vyziadaj chybajuce alebo chybne
	i = 0;
	while (i < frag_amount) {
		if (string_arr[i][0] == '\0') {														
			my_data = request_fragment(i, socket_file_descriptor, sender_addr, sender_size, &checksum_original, &checksum_received);
			frag_number = my_data.header.FRAG_NUMBER;
			strcpy(string_arr[frag_number], my_data.message);
		}
		i++;
	}

	// vypis spravu
	i = 0;
	printf("prijata sprava:\n\n");
	while (i < frag_amount) {
		printf("%s", string_arr[i]);
		i++;
	}
	printf("\n\nkoniec prijatej spravy\n\n");

	//posli potvrdenie
	struct received_header received;
	received.TYPE = 5;
	received.CONFIRMATION = 1;
	received.FRAG_NUMBER = 0;
	sendto(socket_file_descriptor, (char*)&received, sizeof(received_header), 0, (SOCKADDR *)&sender_addr, sender_size);

	check = closesocket(socket_file_descriptor);
	if (check == SOCKET_ERROR) {
		printf("Nepodarilo sa zatvorit socket. Error cislo: %d.\n", WSAGetLastError());
		exit(EXIT_FAILURE);
	}

	check = WSACleanup();
	if (check) {
		printf("Nepodarilo sa zatvorit WSA. Error cislo: %d.\n", WSAGetLastError());
		exit(EXIT_FAILURE);
	}

}

void receive_file() {
	int frag_amount, frag_number, frag_size, fragments_received, i, check, sender_addr_size, sender_size, descriptor_ready;
	char **string_arr, file_name[1457];
	struct sockaddr_in sender_addr;
	WSADATA winsock_stuff;
	SOCKET socket_file_descriptor;
	short port;
	struct data_to_send my_data;
	uint32_t checksum_original, checksum_received;
	FILE *fw;
	fd_set file_descriptors, file_descriptors_copy;
	struct timeval timer;

	file_name[0] = '\0';

	printf("Zadajte prosim cislo portu.\n");
	scanf("%hd", &port);
	check = WSAStartup(MAKEWORD(2, 2), &winsock_stuff);
	if (check) {
		printf("Bohuzial, nepodarilo sa rozbehat winsock. Tu je cislo daneho erroru: %d.\n"
			"Vela stastia pri jeho rieseni. Kym ho najdete, program sa zatial vypne. Ked budete mat riesenie, nevahajte ho opat spustit.", check);
		exit(EXIT_FAILURE);
	}

	socket_file_descriptor = INVALID_SOCKET;
	socket_file_descriptor = socket(AF_INET, SOCK_DGRAM, 0); 
	if (socket_file_descriptor == INVALID_SOCKET) {
		printf("ERROR: nepodarilo sa vytvorit socket. Program sa nasledne sam vypne. Error cislo: %d.\n", WSAGetLastError());
		getchar();
		exit(EXIT_FAILURE);
	}

	sender_addr.sin_family = AF_INET;
	sender_addr.sin_addr.S_un.S_addr = INADDR_ANY;
	sender_addr.sin_port = htons(port);

	check = bind(socket_file_descriptor, (SOCKADDR *)&sender_addr, sizeof(sender_addr));
	if (check) {
		printf("Z neznameho dovodu zlyhal bind. Error cislo: %d.\n", WSAGetLastError());
		exit(EXIT_FAILURE);
	}
	printf("Cakam na spravu od prijmatela.\n");

	sender_size = sizeof(sender_addr);
	frag_number = 0;
	frag_amount = 0;
	fragments_received = 0;


	FD_ZERO(&file_descriptors);
	FD_SET(socket_file_descriptor, &file_descriptors);
	timer.tv_sec = 10;
	timer.tv_usec = 0;

	while (1) {
		file_descriptors_copy = file_descriptors;
		descriptor_ready = select(socket_file_descriptor, &file_descriptors_copy, NULL, NULL, &timer);
		if (descriptor_ready == 0)
		{
			printf("Prvy packet zatial neprisiel, cakam na dalsi.\n");
		}
		else if (descriptor_ready == -1)
		{
			printf("Select vyhodil chybu cislo: %d.\n", WSAGetLastError());
			exit(EXIT_FAILURE);
		}
		else {
			check = recvfrom(socket_file_descriptor, (char*)&my_data, sizeof(data_to_send), 0, (SOCKADDR *)&sender_addr, &sender_size);
			if (check == SOCKET_ERROR) {
				printf("Houston, mame problem. Prijem sa nepodaril. Error cislo: %d.\n", WSAGetLastError());
				exit(EXIT_FAILURE);
			}

			checksum_original = my_data.header.CHECKSUM;
			checksum_received = crc32b((unsigned char*)&my_data);
			if (checksum_received == checksum_original)
				break;
			if (my_data.header.TYPE == 2)
				fragments_received++;
		}
	}

	frag_size = my_data.header.FRAG_SIZE;
	frag_amount = my_data.header.FRAG_AMOUNT;
	frag_number = my_data.header.FRAG_NUMBER;
	string_arr = (char**)malloc(frag_amount * sizeof(char*));
	for (i = 0; i < frag_amount; i++)
		string_arr[i] = (char*)malloc(frag_size + 1);
	for (i = 0; i < frag_amount; i++)
		string_arr[i][0] = '\0';

	printf("prijate fragmenty budu mat max velkost %dB\n", frag_size);

	if (my_data.header.TYPE == 2) {
		strcpy(string_arr[frag_number], my_data.message);
		fragments_received++;
	}
	else
		strcpy(file_name, my_data.message);

	timer.tv_sec = 0;
	timer.tv_usec = 100;
	while (fragments_received != frag_amount) {
		file_descriptors_copy = file_descriptors;
		FD_ZERO(&file_descriptors_copy);
		FD_SET(socket_file_descriptor, &file_descriptors_copy);
		descriptor_ready = select(socket_file_descriptor, &file_descriptors_copy, NULL, NULL, &timer);
		if (descriptor_ready == 0)
		{
			fragments_received++;
		}
		else if (descriptor_ready == -1)
		{
			printf("Select vyhodil chybu cislo: %d.\n", WSAGetLastError());
			exit(EXIT_FAILURE);
		}
		else {
			check = recvfrom(socket_file_descriptor, (char*)&my_data, sizeof(my_data), 0, (SOCKADDR *)&sender_addr, &sender_size);
			if (check == SOCKET_ERROR) {
				printf("Houston, mame problem. Prijem sa nepodaril. Error cislo: %d.\n", WSAGetLastError());
				exit(EXIT_FAILURE);
			}

			checksum_received = crc32b((unsigned char*)&my_data);
			checksum_original = my_data.header.CHECKSUM;
			if (checksum_received != checksum_original) {
				fragments_received++;
				continue;
			}

			frag_number = my_data.header.FRAG_NUMBER;

			if (my_data.header.TYPE == 2) {
				strcpy(string_arr[frag_number], my_data.message);
				fragments_received++;
			}
			else
				strcpy(file_name, my_data.message);
		}
	}

	// teraz prejdi polom prijatych a zisti ci nejaky nechyba

	i = 0;
	while (i < frag_amount) {
		if (string_arr[i][0] == '\0') {
			my_data = request_fragment(i, socket_file_descriptor, sender_addr, sender_size, &checksum_original, &checksum_received);
			frag_number = my_data.header.FRAG_NUMBER;
			strcpy(string_arr[frag_number], my_data.message);
		}
		i++;
	}


	// realne vytvorenie suboru
	if ((fw = fopen(file_name, "w")) == NULL) {
		printf("Nepodarilo sa otvorit subor na zapis.\n");
		exit(EXIT_FAILURE);
	}

	// pre skusobne ucely --> aby som si nepresisal subor z ktoreho citam
	//if ((fw = fopen("test.txt", "w")) == NULL) {
	//	printf("Subor sa nepodarilo otvoprit na zapis.\n");
	//	exit(EXIT_FAILURE);
	//}

	i = 0;
	while (i < frag_amount) {
		if (i == frag_amount - 1)
			fwrite(string_arr[i], 1, strlen(string_arr[i]), fw);
		else
			fwrite(string_arr[i], 1, frag_size, fw);
		i++;
	}

	printf("uspesne sa prijal subor ---%s---. nachadza sa v tom istom priecinku ako program. \n", file_name);
	//posli potvrdenie
	struct received_header received;
	received.TYPE = 5;
	received.CONFIRMATION = 1;
	received.FRAG_NUMBER = 0;
	sendto(socket_file_descriptor, (char*)&received, sizeof(received_header), 0, (SOCKADDR *)&sender_addr, sender_size);

	check = closesocket(socket_file_descriptor);
	if (check == SOCKET_ERROR) {
		printf("Nepodarilo sa zatvorit socket. Error cislo: %d.\n", WSAGetLastError());
		exit(EXIT_FAILURE);
	}

	check = WSACleanup();
	if (check) {
		printf("Nepodarilo sa zatvorit WSA. Error cislo: %d.\n", WSAGetLastError());
		exit(EXIT_FAILURE);
	}

	fclose(fw);
}


void send_text(char* ip_address, int port, int frag_size) {
	int frag_amount, frag_number, message_length, i, position, check, receiver_size, descriptor_ready;
	char message[10000], **string_arr;
	struct sockaddr_in receiver_addr;
	WSADATA winsock_stuff;
	SOCKET socket_file_descriptor;
	struct data_to_send sending;
	fd_set file_descriptors, file_descriptors_copy;
	struct timeval timer;
	char end_str[] = { "_END_" };

	printf("Prosim, napiste spravu ktoru chcete odoslat. Stlacenim enteru ju odoslete.\n");
	getchar();

	fgets(message, 10000, stdin);
	message[strcspn(message, "\r\n")] = 0; // odstranenie newlinu zo spravy. zdroj ---> https://stackoverflow.com/questions/2693776/removing-trailing-newline-character-from-fgets-input
	message_length = strlen(message) + 1; // +1 lebo musim poslat aj null terminator
	frag_amount = (message_length + frag_size - 1 + strlen(end_str)) / frag_size; // delenie so zaokruhlenim nahor --> pocet fragmentov musi byt najblizsie vacsie cele cislo. zdroj ---> https://stackoverflow.com/questions/2745074/fast-ceiling-of-an-integer-division-in-c-c
	frag_number = 0;
	
	strcat(message, end_str);

	string_arr = (char**)malloc(frag_amount * sizeof(char*));
	for (i = 0; i < frag_amount; i++) {
		string_arr[i] = (char*)malloc(frag_size + 1);
	}

	check = WSAStartup(MAKEWORD(2, 2), &winsock_stuff);
	if (check) {
		printf("Bohuzial, nepodarilo sa rozbehat winsock. Tu je cislo daneho erroru: %d.\n"
			"Vela stastia pri jeho rieseni. Kym ho najdete, program sa zatial vypne. Ked budete mat riesenie, nevahajte ho opat spustit.", check);
		exit(EXIT_FAILURE);
	}

	socket_file_descriptor = INVALID_SOCKET;
	socket_file_descriptor = socket(AF_INET, SOCK_DGRAM, 0);
	if (socket_file_descriptor == INVALID_SOCKET) {
		printf("ERROR: nepodarilo sa vytvorit socket. Program sa nasledne sam vypne. Error cislo: %d.\n", WSAGetLastError());
		getchar();
		exit(EXIT_FAILURE);
	}

	receiver_addr.sin_family = AF_INET;
	receiver_addr.sin_addr.S_un.S_addr = inet_addr(ip_address);
	receiver_addr.sin_port = htons(port);
	receiver_size = sizeof(receiver_addr);

	//fragmentacia
	position = 0;

	while (position < strlen(message) + 1) {
		strncpy(string_arr[frag_number], message + position, frag_size);
		string_arr[frag_number][frag_size] = '\0';
		position += frag_size;

		sending.header.TYPE = 1;
		sending.header.FRAG_AMOUNT = frag_amount;
		sending.header.FRAG_NUMBER = frag_number;
		sending.header.FRAG_SIZE = frag_size;
		sending.header.CHECKSUM = crc32b((unsigned char*)&sending);
		strcpy(sending.message, string_arr[frag_number]);


		// odosielanie
		check = sendto(socket_file_descriptor, (char *)&sending, sizeof(data_to_send), 0, (SOCKADDR *)&receiver_addr, receiver_size); // posledny sizof nahradit premennou do ktorej to nasupujem?
		if (check == SOCKET_ERROR) {
			printf("Bohuzial, spravu sa nepodarilo odoslat. Error cislo: %d.\n", WSAGetLastError());
			return;
		}
		frag_number++;
	}

	//prijmi potvrdenie
	struct received_header received;
	received.CONFIRMATION = 0;
	FD_ZERO(&file_descriptors);
	FD_SET(socket_file_descriptor, &file_descriptors);
	timer.tv_sec = 0;
	timer.tv_usec = 100;
	while (received.CONFIRMATION != 1) { 
		file_descriptors_copy = file_descriptors;
		descriptor_ready = select(socket_file_descriptor, &file_descriptors_copy, NULL, NULL, &timer);
		if (descriptor_ready == 0)
		{
			//printf("Packet sa nedostavil.\n");
		}
		else if (descriptor_ready == -1)
		{
			printf("Select vyhodil chybu cislo: %d.\n", WSAGetLastError());
			exit(EXIT_FAILURE);
		}
		else {
			check = recvfrom(socket_file_descriptor, (char*)&received, sizeof(received_header), 0, (SOCKADDR *)&receiver_addr, &receiver_size);
			if (received.CONFIRMATION == 1) {
				printf("Vasa sprava bola uspesne prijata.\n");
				break;
			}

			sending.header.TYPE = 1;
			sending.header.FRAG_AMOUNT = frag_amount;
			sending.header.FRAG_NUMBER = received.FRAG_NUMBER;
			sending.header.FRAG_SIZE = frag_size;
			sending.header.CHECKSUM = crc32b((unsigned char*)&sending);
			strcpy(sending.message, string_arr[received.FRAG_NUMBER]);
			check = sendto(socket_file_descriptor, (char *)&sending, sizeof(data_to_send), 0, (SOCKADDR *)&receiver_addr, receiver_size);
			if (check == SOCKET_ERROR) {
				printf("Bohuzial, spravu sa nepodarilo odoslat. Error cislo: %d.\n", WSAGetLastError());
				return;
			}
		}
	}


	check = closesocket(socket_file_descriptor);
	if (check == SOCKET_ERROR) {
		printf("Nepodarilo sa zatvorit socket. Error cislo: %d.\n", WSAGetLastError());
		exit(EXIT_FAILURE);
	}

	check = WSACleanup();
	if (check) {
		printf("Nepodarilo sa zatvorit WSA. Error cislo: %d.\n", WSAGetLastError());
		exit(EXIT_FAILURE);
	}

}

void send_corrupted(char* ip_address, int port, int frag_size) {
	int frag_amount, frag_number, message_length, i, position, check, receiver_size, descriptor_ready;
	char message[1000], **string_arr;
	struct sockaddr_in receiver_addr;
	WSADATA winsock_stuff;
	SOCKET socket_file_descriptor;
	struct data_to_send sending;
	fd_set file_descriptors, file_descriptors_copy;
	struct timeval timer;

	printf("Prosim, napiste spravu ktoru chcete odoslat. Stlacenim enteru ju odoslete.\n");
	getchar();

	fgets(message, 1000, stdin);
	message[strcspn(message, "\r\n")] = 0; // odstranenie newlinu zo spravy. zdroj ---> https://stackoverflow.com/questions/2693776/removing-trailing-newline-character-from-fgets-input
	message_length = strlen(message) + 1; // +1 lebo musim poslat aj null terminator
	frag_amount = (message_length + frag_size - 1) / frag_size; // delenie so zaokruhlenim nahor --> pocet fragmentov musi byt najblizsie vacsie cele cislo. zdroj ---> https://stackoverflow.com/questions/2745074/fast-ceiling-of-an-integer-division-in-c-c
	frag_number = 0;
	string_arr = (char**)malloc(frag_amount * sizeof(char*));
	for (i = 0; i < frag_amount; i++) {
		string_arr[i] = (char*)malloc(frag_size + 1);
	}

	check = WSAStartup(MAKEWORD(2, 2), &winsock_stuff);
	if (check) {
		printf("Bohuzial, nepodarilo sa rozbehat winsock. Tu je cislo daneho erroru: %d.\n"
			"Vela stastia pri jeho rieseni. Kym ho najdete, program sa zatial vypne. Ked budete mat riesenie, nevahajte ho opat spustit.", check);
		exit(EXIT_FAILURE);
	}

	socket_file_descriptor = INVALID_SOCKET;
	socket_file_descriptor = socket(AF_INET, SOCK_DGRAM, 0);
	if (socket_file_descriptor == INVALID_SOCKET) {
		printf("ERROR: nepodarilo sa vytvorit socket. Program sa nasledne sam vypne. Error cislo: %d.\n", WSAGetLastError());
		getchar();
		exit(EXIT_FAILURE);
	}

	receiver_addr.sin_family = AF_INET;
	receiver_addr.sin_addr.S_un.S_addr = inet_addr(ip_address);
	receiver_addr.sin_port = htons(port);
	receiver_size = sizeof(receiver_addr);

	//fragmentacia
	position = 0;

	while (position < strlen(message) + 1) {
		strncpy(string_arr[frag_number], message + position, frag_size);
		string_arr[frag_number][frag_size] = '\0';

		sending.header.TYPE = 1;
		sending.header.FRAG_AMOUNT = frag_amount;
		sending.header.FRAG_NUMBER = frag_number;
		sending.header.FRAG_SIZE = frag_size;
		if (position == 0)
			sending.header.CHECKSUM = 0;
		else
			sending.header.CHECKSUM = crc32b((unsigned char*)&sending);
		strcpy(sending.message, string_arr[frag_number]);


		// odosielanie
		check = sendto(socket_file_descriptor, (char *)&sending, sizeof(data_to_send), 0, (SOCKADDR *)&receiver_addr, receiver_size); // posledny sizof nahradit premennou do ktorej to nasupujem?
		if (check == SOCKET_ERROR) {
			printf("Bohuzial, spravu sa nepodarilo odoslat. Error cislo: %d.\n", WSAGetLastError());
			return;
		}
		frag_number++;
		position += frag_size;
	}

	//prijmi potvrdenie
	struct received_header received;
	received.CONFIRMATION = 0;
	FD_ZERO(&file_descriptors);
	FD_SET(socket_file_descriptor, &file_descriptors);
	timer.tv_sec = 0;
	timer.tv_usec = 100;
	while (received.CONFIRMATION != 1) {
		file_descriptors_copy = file_descriptors;
		descriptor_ready = select(socket_file_descriptor, &file_descriptors_copy, NULL, NULL, &timer);
		if (descriptor_ready == 0)
		{
			//printf("Kde je moj packet???.\n");
		}
		else if (descriptor_ready == -1)
		{
			printf("Select vyhodil chybu, znamo preco. Error cislo: %d.\n", WSAGetLastError());
			exit(EXIT_FAILURE);
		}
		else {
			check = recvfrom(socket_file_descriptor, (char*)&received, sizeof(received_header), 0, (SOCKADDR *)&receiver_addr, &receiver_size);
			if (received.CONFIRMATION == 1) {
				printf("Vasa sprava bola uspesne prijata.\n");
				break;
			}

			sending.header.TYPE = 1;
			sending.header.FRAG_AMOUNT = frag_amount;
			sending.header.FRAG_NUMBER = received.FRAG_NUMBER;
			sending.header.FRAG_SIZE = frag_size;
			sending.header.CHECKSUM = crc32b((unsigned char*)&sending);
			strcpy(sending.message, string_arr[received.FRAG_NUMBER]);
			check = sendto(socket_file_descriptor, (char *)&sending, sizeof(data_to_send), 0, (SOCKADDR *)&receiver_addr, receiver_size);
			if (check == SOCKET_ERROR) {
				printf("Bohuzial, spravu sa nepodarilo odoslat. Error cislo: %d.\n", WSAGetLastError());
				return;
			}
		}
	}

	check = closesocket(socket_file_descriptor);
	if (check == SOCKET_ERROR) {
		printf("Nepodarilo sa zatvorit socket. Error cislo: %d.\n", WSAGetLastError());
		exit(EXIT_FAILURE);
	}

	check = WSACleanup();
	if (check) {
		printf("Nepodarilo sa zatvorit WSA. Error cislo: %d.\n", WSAGetLastError());
		exit(EXIT_FAILURE);
	}

}

void send_file(char* ip_address, int port, int frag_size) {
	int frag_amount, frag_number, message_length, i, position, check, receiver_size, descriptor_ready;
	char *buffer, **string_arr, file_name[1457];
	struct sockaddr_in receiver_addr;
	WSADATA winsock_stuff;
	SOCKET socket_file_descriptor;
	FILE *fr;
	struct data_to_send sending;
	fd_set file_descriptors, file_descriptors_copy;
	struct timeval timer;

	printf("Zadajte prosim nazov suboru. Berte na vedomie, ze odosielat sa daju iba textove subory. Subor musi byt ulozeny tam kde aj program.\n");
	scanf("%s", file_name);

	if ((fr = fopen(file_name, "r")) == NULL) {
		printf("Nepodarilo sa otvorit subor na odoslanie.\n");
		exit(EXIT_FAILURE);
	}

	// zistit pocet fragmentov --> prejst najprv cely subor a zvacsovat ho
	buffer = (char*)malloc(frag_size);
	frag_amount = 0;
	while (fread(buffer, 1, frag_size, fr) != 0) {
		frag_amount++;
	}
	rewind(fr);

	string_arr = (char**)malloc(frag_amount * sizeof(char*));
	for (i = 0; i < frag_amount; i++) {
		string_arr[i] = (char*)malloc(frag_size + 1);
	}

	check = WSAStartup(MAKEWORD(2, 2), &winsock_stuff);
	if (check) {
		printf("Bohuzial, nepodarilo sa rozbehat winsock. Tu je cislo daneho erroru: %d.\n"
			"Vela stastia pri jeho rieseni. Kym ho najdete, program sa zatial vypne. Ked budete mat riesenie, nevahajte ho opat spustit.", check);
		exit(EXIT_FAILURE);
	}

	socket_file_descriptor = INVALID_SOCKET;
	socket_file_descriptor = socket(AF_INET, SOCK_DGRAM, 0);
	if (socket_file_descriptor == INVALID_SOCKET) {
		printf("ERROR: nepodarilo sa vytvorit socket. Program sa nasledne sam vypne. Error cislo: %d.\n", WSAGetLastError());
		getchar();
		exit(EXIT_FAILURE);
	}

	receiver_addr.sin_family = AF_INET;
	receiver_addr.sin_addr.S_un.S_addr = inet_addr(ip_address);
	receiver_addr.sin_port = htons(port);
	receiver_size = sizeof(receiver_addr);

	sending.header.TYPE = 3;
	sending.header.FRAG_AMOUNT = frag_amount;
	sending.header.FRAG_NUMBER = 0;
	sending.header.FRAG_SIZE = frag_size;
	sending.header.CHECKSUM = crc32b((unsigned char*)&sending);
	strcpy(sending.message, file_name);
	check = sendto(socket_file_descriptor, (char *)&sending, sizeof(data_to_send), 0, (SOCKADDR *)&receiver_addr, receiver_size); 
	if (check == SOCKET_ERROR) {
		printf("Bohuzial, spravu sa nepodarilo odoslat. Error cislo: %d.\n", WSAGetLastError());
		return;
	}

	frag_number = 0;
	int vololo = 0;
	while (1) { 

		memset(buffer, '\0', frag_size);
		vololo = fread(buffer, 1, frag_size, fr);
		if (vololo <= 0)
			break;
		strcpy(string_arr[frag_number], buffer);
		string_arr[frag_number][frag_size] = '\0';
		sending.header.TYPE = 2;
		sending.header.FRAG_AMOUNT = frag_amount;
		sending.header.FRAG_NUMBER = frag_number;
		sending.header.FRAG_SIZE = (uint32_t)vololo;
		sending.header.CHECKSUM = crc32b((unsigned char*)&sending);
		strcpy(sending.message, string_arr[frag_number]);

		// odosielanie
		check = sendto(socket_file_descriptor, (char *)&sending, sizeof(data_to_send), 0, (SOCKADDR *)&receiver_addr, receiver_size); 
		if (check == SOCKET_ERROR) {
			printf("Bohuzial, spravu sa nepodarilo odoslat. Error cislo: %d.\n", WSAGetLastError());
			return;
		}

		frag_number++;

	}

	//prijmi potvrdenie
	struct received_header received;
	received.CONFIRMATION = 0;
	FD_ZERO(&file_descriptors);
	FD_SET(socket_file_descriptor, &file_descriptors);
	timer.tv_sec = 0;
	timer.tv_usec = 100;
	while (received.CONFIRMATION != 1) { 
		file_descriptors_copy = file_descriptors;
		descriptor_ready = select(socket_file_descriptor, &file_descriptors_copy, NULL, NULL, &timer);
		if (descriptor_ready == 0)
		{
			//printf("Packet sa nedostavil.\n");
		}
		else if (descriptor_ready == -1)
		{
			printf("Select vyhodil chybu cislo: %d.\n", WSAGetLastError());
			exit(EXIT_FAILURE);
		}
		else {
			check = recvfrom(socket_file_descriptor, (char*)&received, sizeof(received_header), 0, (SOCKADDR *)&receiver_addr, &receiver_size);
			if (received.CONFIRMATION == 1) {
				printf("Subor bol uspesne prijaty.\n");
				break;
			}

			sending.header.TYPE = 1;
			sending.header.FRAG_AMOUNT = frag_amount;
			sending.header.FRAG_NUMBER = received.FRAG_NUMBER;
			sending.header.FRAG_SIZE = frag_size;
			sending.header.CHECKSUM = crc32b((unsigned char*)&sending);
			strcpy(sending.message, string_arr[received.FRAG_NUMBER]);
			check = sendto(socket_file_descriptor, (char *)&sending, sizeof(data_to_send), 0, (SOCKADDR *)&receiver_addr, receiver_size);
			if (check == SOCKET_ERROR) {
				printf("Bohuzial, spravu sa nepodarilo odoslat. Error cislo: %d.\n", WSAGetLastError());
				return;
			}
		}
	}

	check = closesocket(socket_file_descriptor);
	if (check == SOCKET_ERROR) {
		printf("Nepodarilo sa zatvorit socket. Error cislo: %d.\n", WSAGetLastError());
		exit(EXIT_FAILURE);
	}

	check = WSACleanup();
	if (check) {
		printf("Nepodarilo sa zatvorit WSA. Error cislo: %d.\n", WSAGetLastError());
		exit(EXIT_FAILURE);
	}

	fclose(fr);

}

struct data_to_send request_fragment(int frag_number, SOCKET socket_file_descriptor,
	struct sockaddr_in sender_addr,
	int sender_size, uint32_t *checksum_original, uint32_t *checksum_received) {

	int check, descriptor_ready;
	struct data_to_send tmp;
	struct received_header received;
	fd_set file_descriptors, file_descriptors_copy;
	struct timeval timer;
	received.TYPE = 5;
	received.CONFIRMATION = 0;
	received.FRAG_NUMBER = frag_number;

	tmp.header.CHECKSUM = 0;
	sendto(socket_file_descriptor, (char*)&received, sizeof(received_header), 0, (SOCKADDR *)&sender_addr, sender_size);
	
	FD_ZERO(&file_descriptors);
	FD_SET(socket_file_descriptor, &file_descriptors);
	timer.tv_sec = 0;
	timer.tv_usec = 100;
	while (1) {
		file_descriptors_copy = file_descriptors;
		descriptor_ready = select(socket_file_descriptor, &file_descriptors_copy, NULL, NULL, &timer);
		if (descriptor_ready == 0)
		{
			//printf("Packet sa nedostavil.\n");
		}
		else if (descriptor_ready == -1)
		{
			printf("Select vyhodil chybu cislo: %d.\n", WSAGetLastError());
			exit(EXIT_FAILURE);
		}
		else {
			check = recvfrom(socket_file_descriptor, (char*)&tmp, sizeof(data_to_send), 0, (SOCKADDR *)&sender_addr, &sender_size);
			if (check == SOCKET_ERROR) {
				printf("Houston, mame problem. Prijem sa nepodaril. Error cislo: %d.\n", WSAGetLastError());
				exit(EXIT_FAILURE);
			}
			*checksum_original = tmp.header.CHECKSUM;
			*checksum_received = crc32b((unsigned char*)&tmp);
			if (*checksum_received == *checksum_original) 
				return tmp;
			else
				tmp = request_fragment(frag_number, socket_file_descriptor, sender_addr, sender_size, checksum_original, checksum_received);
			return tmp;
		}
	}
}


