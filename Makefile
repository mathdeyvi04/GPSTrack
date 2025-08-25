#################################################################
## Variáveis de Execução
#################################################################

# Nome do programa
TARGET      := TrackSense

# Fontes do projeto
SRC         := src/main.cpp
OBJ         := $(SRC:.cpp=.o)

# IP e usuário da placa
# Ainda precisamos da placa para descobrir mais
PLACA_USER  := 
PLACA_IP    := 
PLACA_PATH  := 

# Local da Pasta Latex
LATEX_PATH  := docs/latex

# Caminhos Ligados À Compilação
CXX         := arm-buildroot-linux-gnueabihf_sdk-buildroot/bin/arm-buildroot-linux-gnueabihf-g++
SYSROOT     := arm-buildroot-linux-gnueabihf_sdk-buildroot/arm-buildroot-linux-gnueabihf/sysroot
CXXFLAGS    := --sysroot=$(SYSROOT) -Wall -O2



#################################################################
## Comandos de Execução
#################################################################

# Executando de forma geral
all:
	@echo "\e[1;36;40m[INFO] Buildando Binário Para Placa...\e[0m"
	@$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET)

# Executando de forma a debugar nosso código.
debug:
	@echo "\e[1;36;40m[INFO] Buildando e Executando Binário Para Debugação...\e[0m"
	@g++ src/debug.cpp -o debug; ./debug; rm -f debug;


# Enviando Programas Para Placa
deploy: $(TARGET)
	@echo "\e[1;36;40m[INFO] Enviando Binário Para Placa...\e[0m"
	scp $(TARGET) $(PLACA_USER)@$(PLACA_IP):$(PLACA_PATH)/
	@echo "\e[1;36;40m[INFO] Ajustando Permissão de Execução...\e[0m"
	ssh $(PLACA_USER)@$(PLACA_IP) \
		"chmod +x $(PLACA_PATH)/$(TARGET)"
	# Adicionar '&& $(PLACA_PATH)/$(TARGET)'
	# executará o programa na placa


# Gerando Documentação
docs:
	@echo "\e[1;36;40m[INFO] Gerando HTML e LATEX com Doxygen\e[0m"
	@doxygen Doxyfile
	@echo "\e[1;36;40m[INFO] Compilando PDF na pasta docs/latex\e[0m"
	@$(MAKE) -C $(LATEX_PATH)
	@echo "\e[1;36;40m[INFO] Trazendo PDF para diretório padrão\e[0m"
	@mv $(LATEX_PATH)/refman.pdf Documentation.pdf


# Limpamos 
clean:
	@rm -rf docs/html docs/latex 


.PHONY: docs