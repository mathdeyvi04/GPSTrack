#################################################################
## Variáveis de Execução
#################################################################

# Nome do programa
TARGET      := GPSTracker

# Fontes do projeto
SRC         := src/main.cpp
OBJ         := $(SRC:.cpp=.o)

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
build:
	@echo "\e[1;36m[INFO] Buildando Binário Para Placa...\e[0m"
	@$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET)

.PHONY: debug
debug:
	@echo "\e[1;36m[INFO] Buildando e Executando Binário Para Debugação...\e[0m"
	@g++ src/debug.cpp -o debug; ./debug $(IP) $(PORTA); rm -f debug;

.PHONY: docs
docs:
	@echo "\e[1;36m[INFO] Gerando HTML e LATEX com Doxygen\e[0m"
	@doxygen Doxyfile
	@echo "\e[1;36m[INFO] Compilando PDF na pasta docs/latex\e[0m"
	@$(MAKE) -C $(LATEX_PATH)
	@echo "\e[1;36m[INFO] Trazendo PDF para diretório padrão\e[0m"
	@mv $(LATEX_PATH)/refman.pdf Documentation.pdf

.PHONY: showup
showup:
	@if [ -z "$$VIRTUAL_ENV" ]; then \
		echo "❌ Ambiente virtual não está ativado!"; \
		echo "Ative com: source venv/bin/activate"; \
		exit 1; \
	else \
		echo "\e[1;36m[INFO] Apresentando Monitor\e[0m"; \
		streamlit run src/GPSMonitor.py; \
	fi

.PHONY: clean
clean:
	@rm -rf docs/html docs/latex 
