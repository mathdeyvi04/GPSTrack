# Sumário

- [Objetivo](#objetivo)
- [Caso Deseje **Contribuir**](#caso-deseje-contribuir)
- [Caso Deseje **Utilizar**](#caso-deseje-utilizar)
- [Descrições Sucintas de Código](#descrições-sucintas-de-código)

# Objetivo

Desenvolver uma solução embarcada usando o kit de desenvolvimento **STM32MP1 DK1** e
o sensor _GPS_, a fim de monitorar de forma remota e contínua qualquer tentativa de violação e/ou comprometimento de uma carga.

Em termos mais técnicos, a aplicação deve conseguir ler e interpretar dados obtidos pelo sensor e enviá-los via UDP socket para
um determinado servidor.

# Caso Deseje Contribuir

Supondo que você deseje ser um contribuinte, atente-se às necessidades da aplicação:

- Compilador C++ **Cross Operacional**

Além do compilador padrão para verificações superficiais, será necessário um compilador específico
para o módulo que estamos focando:

```
sudo apt install g++-arm-linux-gnueabihf
```
- Makefile

Caso fôssemos utilizar apenas o compilador, precisaríamos escrever comandos grandes e fácieis de 
errar. Sendo assim, faz-se necessário um intermediário:

```
sudo apt install make
```

- Doxygen

Utilizaremos esse software para criar documentações de forma automática.

```
sudo apt install doxygen graphviz texlive-latex-base
```



... -> Sucessivas necessidades e dependências do sensor.

# Caso Deseje Utilizar

A seguir, regras de compilação e execução.

### `TrackSense`

O arquivo binário `TrackSense` presente neste diretório já representa o executável a ser inserido no módulo para completo
funcionamento do módulo.

### `make`

Compilará a aplicação utilizando as flags necessárias e o compilador correto, gerando 
um executável de pronto uso no módulo.

> [!TIP]
> Este caso é supondo que você deseje contribuir e, portanto, possui as dependências necessárias.


# Descrições Sucintas de Código

...

Caso deseje uma maior profundidade nas explicações, verifique [docs](docs)
