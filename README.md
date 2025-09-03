# Sumário

- [Objetivo](#objetivo)
- [Caso Deseje **Contribuir**](#caso-deseje-contribuir)
- [Caso Deseje **Utilizar**](#caso-deseje-utilizar)
- [Descrições Sucintas de Código](#descrições-sucintas-de-código)

# Objetivo

Desenvolver uma solução embarcada usando o kit de desenvolvimento **STM32MP1 DK1** e
o sensor **GY-GPS6MV2**, a fim de monitorar de forma remota e contínua qualquer tentativa de violação e/ou comprometimento de uma carga.

Em termos mais técnicos, a aplicação deve conseguir ler e interpretar dados obtidos pelo sensor e enviá-los via UDP para um determinado servidor.

# Caso Deseje Contribuir

Supondo que você deseje ser um contribuinte, atente-se às necessidades da aplicação:

### Conhecimentos a cerca do [Módulo GPS **GY-GPS6MV2**](https://youtu.be/lZumBl7zhoM):

Sugiro a observação do vídeo linkado e busca inteligente no chatgpt.

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

Demais funcionalidades são explicadas dentro do arquivo Makefile. Utilizaremos o `...g++`.

### Makefile

Caso fôssemos utilizar apenas o compilador, precisaríamos escrever comandos grandes e fáceis de 
errar. Sendo assim, faz-se necessário um intermediário:

```
sudo apt install make
```

### Doxygen

Utilizaremos esse software para criar documentações de forma automática. 
**Não é necessário que todos os contribuintes possuam essa ferramenta**,
entretanto, é estritamente obrigatório a utilização do mesmo padrão de documentação.

```
sudo apt install doxygen graphviz
sudo apt install texlive-latex-extra texlive-fonts-recommended texlive-fonts-extra texlive-latex-recommended
```

O segundo download faz referência ao construção do arquivo `.pdf`.

### Acesso à Placa

Utilizaremos o protocolo SSH para acessá-la remotamente. Naturalmente,
será necessário um cabo Ethernet e que os IP's estejam na mesma faixa.
É interessante que você utilize a função _ping_ para verificar se estão conectadas corretamente.

Assim que a conexão for confirmada, dentro do PowerShell, execute:

```
ssh root@192.168.42.2
```

Será solicitado a senha e, posteriormente, o acesso será permitido.

Para enviar arquivos para a placa, volte novamente ao PowerShell e:

```
scp TrackSense root@192.168.42.2:/
```

Assim, o arquivo estará no mesmo local que as pastas root do sistema linux. Além disso, pode ser necessário dar permissão de execução ao binário, para tanto:

```
chmod +x TrackSense
```
# Caso Deseje Utilizar

A seguir, regras de compilação e execução.

### `TrackSense`

O arquivo binário `TrackSense` presente neste diretório já representa o executável a ser inserido no módulo para completo
funcionamento do módulo.

### `make`

Compilará a aplicação utilizando as flags necessárias e o compilador específico, gerando 
um executável de pronto uso no módulo.

### `make debug` 

Compilará a aplicação para o Linux, executará e apagará o executável gerado.
Neste caso, há um simulador para o módulo GPS que estaremos usando a fim de que 
possamos realmente realizar testes.

### `make docs`

Para contribuintes, gerará um PDF contendo a documentação da aplicação geral.

# Descrições Sucintas de Código

### TrackSense

Classe responsável por:

- Ler dados do sensor GPS6MV2
- Interpretar os dados lidas
- Enviar as informações 

Cada uma dessas fases ocorrem em uma thread dedicada controlada por métodos `init` e `stop`.

Fluxo de Funcionamento:

- Inicialização

A classe configura um socket UDP para a transmissão dos dados processados e uma porta serial, na qual o sensor GPS 6MV2 enviará os dados.

- Leitura dos Dados:

O método `_ler_dados` coleta os dados diretamente da porta serial em blocos de até 100 caracteres. Caso não haja dados, retornará string vazia.

- Interpretação dos Dados:

A classe distingue os dois principais formatos _NMEA_: **RMC** e **GGA**, a partir dos métodos `_parser_rmc` e `_parser_gga`.

- Envio de informações:

_Ainda não desenvolvido_.

Para informações mais precisas e profundas, sugiro verificar o arquivo 
[index.html](docs/html/index.html) ou [Documentation.pdf](Documentation.pdf), sendo este último gerado pelo comando `make docs`.











