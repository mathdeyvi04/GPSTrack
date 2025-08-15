# Gerando Executável Para Módulo
all:


# Gerando Executável Para Testes
debug:

# Gerando Documentação
.PHONY: docs
docs:
	doxygen Doxyfile


# Limpamos documentação
clean:
	@rm -rf docs/html docs/latex







