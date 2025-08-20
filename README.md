# Sumário

- [Objetivo](#objetivo)
- [Caso Deseje **Contribuir**](#caso-deseje-contribuir)
- [Caso Deseje **Utilizar**](#caso-deseje-utilizar)
- [Descrições Sucintas de Código](#descrições-sucintas-de-código)

# Objetivo

Desenvolver uma solução embarcada usando o kit de desenvolvimento **STM32MP1 DK1** e
o sensor _GPS_, a fim de monitorar de forma remota e contínua qualquer tentativa de violação e/ou comprometimento de uma carga.

Em termos mais técnicos, a aplicação deve conseguir ler e interpretar dados obtidos pelo sensor e enviá-los via UDP socket para um determinado servidor.

# Caso Deseje Contribuir

Supondo que você deseje ser um contribuinte, atente-se às necessidades da aplicação:

### Conhecimentos a cerca do [Módulo GPS **GY-GPS6MV2**](https://youtu.be/lZumBl7zhoM):

No arquivo disponibilizado pelo professor, é instruído detalhadamente o passo a passo para utilizar tanto o módulo quanto o kit.

### Kit de Desenvolvimento STM32

Como o professor já detalhou esforçadamente o passo a passo da utilização da placa, focaremos em como permitir que você possa emular o 
compilador necessário.

Além do compilador padrão para verificações superficiais, será necessário um compilador específico para o microcontrolador que estamos focando. 

- Download

Verifique o [link](https://drive.google.com/file/d/1qpq3QeK5f7T061LFA0JlJz2fgMQDvyMn/view?usp=drivesdk) dado pelo professor e realize:

```
tar -xvf arm-buildroot-linux-gnueabihf_sdk-buildroot.tar.gz
```

Isso descompactará o kit de desenvolvimento da placa, permitindo diversas funcionalidades.

> [!WARNING]
> Não se preocupe com o tempo que demorará.

- Sobre o Software STM32MP1 DK1

Ao realizar a descompactação, você terá:

```
arm-buildroot-linux-gnueabihf_sdk-buildroot/
    |-- bin/
    │   |-- arm-buildroot-linux-gnueabihf-gcc
    │   |-- arm-buildroot-linux-gnueabihf-g++
    |   ...
    |-- arm-buildroot-linux-gnueabihf/
    |   |-- sysroot
    |   ...
    |
    ...
```

Demais funcionalidades são explicadas dentro do arquivo Makefile.

### Makefile

Caso fôssemos utilizar apenas o compilador, precisaríamos escrever comandos grandes e fácieis de 
errar. Sendo assim, faz-se necessário um intermediário:

```
sudo apt install make
```

### Doxygen

Utilizaremos esse software para criar documentações de forma automática. 
**Não é necessário que todos os contribuintes possuam essa ferramenta**,
entretanto, é estritamente obrigatório a utilização do mesmo [padrão de documentação](PadrãoDoxygen.md)

```
sudo apt install doxygen graphviz
sudo apt install texlive-latex-extra texlive-fonts-recommended texlive-fonts-extra texlive-latex-recommended
```

O segundo download faz referência ao construção do arquivo .pdf.

### Minicom

Ferramenta de linha de comando que permite abrir portas seriais para comunicação
com dispositivos externos, como microcontroladores, placas de desenvolvimento (ex: STM32).

```
sudo apt install minicom
```

Após conectar a placa:

```
dmesg | grep tty
```

Apresentará algo como `now attached to ttyUSB0`, o que significará que um USB foi conectado
e está registrado em `/dev/ttyUSB0`.

Para abrir a placa, basta executar `sudo minicom`.

Caso seja a primeira vez, podemos configurar as definições da comunicação com `sudo minicom -s`.

... -> Sucessivas necessidades e dependências do sensor.

# Caso Deseje Utilizar

A seguir, regras de compilação e execução.

### `TrackSense`

O arquivo binário `TrackSense` presente neste diretório já representa o executável a ser inserido no módulo para completo
funcionamento do módulo.

### `make`

Compilará a aplicação utilizando as flags necessárias e o compilador específico, gerando 
um executável de pronto uso no módulo.

### `make debug` 

Compilará a aplicação para o Linux, executará e apagará o executável gerável.
Neste caso, há um simulador para o módulo GPS que estaremos usando a fim de que 
possamos realmente realizar testes.

### `make docs`

Gerará um PDF contendo a documentação da aplicação geral.

> [!TIP]
> Estes últimos casos supõem que você deseje contribuir e, portanto, possui as dependências necessárias.


# Descrições Sucintas de Código

...

Caso deseje uma maior profundidade nas explicações, verifique [docs](docs)
