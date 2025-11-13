"""
@file GPSMonitor.py
@brief Implementação da interface de visualização de dados
"""

import base64
import streamlit as st
from streamlit_autorefresh import st_autorefresh
import pandas as pd
import socket
from datetime import datetime
import pytz
import pydeck as pdk

class GPSMonitor:
    """
    @brief Monitor que apresentará os dados do sensor
    @details
    Responsável por:
    - Apresentar os dados do sensor em tempo real
    - Possuir um histórico de dados
    - Possibilitar o salvamento deles
    """

    def __init__(self, host: str, port: int, img=""):
        """
        @brief Construtor da Classe, inicializando atributos cruciais.
        @param host IP da máquina deve receber os pacotes
        @param port Porta de Comunicação
        @param img caminho da imagem de background
        """

        st.session_state["self"] = self
        self.time = []
        self.traject = []
        self.altitude = []
        self.placeholder = st.empty() # Apenas para conseguirmos renderizar o mapa corretamente

        self.zona_brasileira = pytz.timezone("America/Sao_Paulo")
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.bind((host, port))
        self.sock.settimeout(0.5)

        self.img_data = None
        if img:
            with open(
                img,
                "rb"
            ) as f:
                data = f.read()

            self.img_data = base64.b64encode(data).decode()

    def conversion_utc_to_brasil(
            self,
            time_utc: str
    ) -> datetime:
        """
        @brief Converterá a string de time utc para string de time no fuso horário do Brasil
        @param time_utc: String time UTC
        @return String hora em Brasil
        """

        hora_utc = datetime.strptime(time_utc, "%H%M%S").time()
        data_hora_utc = datetime.combine(datetime.now().date(), hora_utc)

        data_hora_utc = pytz.UTC.localize(data_hora_utc)
        data_hora_brasil = data_hora_utc.astimezone(self.zona_brasileira)

        return data_hora_brasil

    def get_data_from_client(
            self
    ) -> None:
        """
        @brief Verifica no buffer do socket o padrão de string setado. Preenchendo as variáveis de estado.
        """

        try:
            data, _ = self.sock.recvfrom(1024)
            numeros = data.decode().strip().split(",")
            print(f"Estou recebendo {numeros}")
            if numeros:
                # Obtendo informações de horário
                self.time.append(
                    {"time": self.conversion_utc_to_brasil(numeros[0][:-3])}
                )

                # Obtendo informações de geolocalização
                self.traject.append(
                    {"latitude": float(numeros[1].strip()), "longitude": float(numeros[2].strip())}
                )

                self.altitude.append(
                    {"altitude": float(numeros[3].strip())}
                )

        except TimeoutError:
            print("Esperando...")

    def set_background(
            self
    ) -> None:
        """
        @brief Setará uma imagem como background da aplicação
        """
        st.markdown(
            f"""
            <style>
            .stApp {{
                background-image: url("data:image/jpg;base64,{self.img_data}");
                background-size: cover;
                background-position: center;
                background-repeat: no-repeat;
            }}
            </style>
            """,
            unsafe_allow_html=True
        )

    def get_all_data(self) -> pd.DataFrame:
        """
        @brief Responsável por prover uma parte do histórico para a tabela apresentada
        """
        return pd.DataFrame(
                    self.time[-15:] # Manteremos sempre no máximo os últimos 15 pontos na tabela
                ).join(
                    pd.DataFrame(
                        self.traject[-15:] # Manteremos sempre no máximo os últimos 15 pontos na tabela
                    ).join(
                        pd.DataFrame(
                            self.altitude[-15:] # Manteremos sempre no máximo os últimos 15 pontos na tabela
                        )
                    )
                )

    def save_historic(self) -> None:
        """
        @brief Salvará dinamicamente os dados que já chegaram.
        """
        with open(
            "historico.csv",
            "w"
        ) as file:
            file.write(
                pd.DataFrame(
                    self.time
                ).join(
                    pd.DataFrame(
                        self.traject
                    ).join(
                        pd.DataFrame(
                            self.altitude
                        )
                    )
                ).to_csv()
            )

    def mainloop(self) -> None:
        """
        @brief Responsável por executar as funcionalidades básicas.
        @details
        Dado o funcionamento do streamlit, executa as funções como se estivessem em loop.
        Verifique como o streamlit executa código python para mais informações.
        """

        self.get_data_from_client()

        st.set_page_config(
            layout="wide"
        )
        st.markdown('<h1 style="color: white;">Paz para Você, Proteção para Sua Carga. Tecnologia a Seu Favor.</h1>', unsafe_allow_html=True)
        if self.traject: # Apenas verificando se não está vazia

            df = pd.DataFrame(
                self.traject
            )

            df["norm"] = df.index / (len(df) - 1 if len(df) > 1 else 1)
            df["norm255"] = (df["norm"] * 255).astype(int)

            lat_center = df["latitude"].mean()
            lon_center = df["longitude"].mean()

            layer = pdk.Layer(
                "ScatterplotLayer",
                data=df,
                get_position='[longitude, latitude]',
                get_color='[235, 140, 52, 255]',
                get_radius=50,
                pickable=True,
                filled=True,
                get_fill_color='[norm255, 0, 255-norm255]'
            )

            view_state = pdk.ViewState(
                latitude=lat_center,
                longitude=lon_center,
                zoom=13,  # ajuste conforme necessário
                pitch=0
            )

            st.pydeck_chart(
                pdk.Deck(
                    layers=[layer], initial_view_state=view_state
                )
            )

            st.dataframe(self.get_all_data())

            if st.button("Salvar Histórico"):
                self.save_historic()
                st.success("Dados Salvos!")

        else:
            st.write("Esperando dados chegarem...")

        if self.img_data:
            self.set_background()
        st_autorefresh(interval=1000, limit=None) # Para recarregarmos a aplicação


if __name__ == '__main__':
    # 192.168.42.10
    # 172.20.38.168
    gps_monitor = GPSMonitor(img="src/antena.jpg", host="172.20.38.168", port=5000) if "self" not in st.session_state else st.session_state["self"]
    gps_monitor.mainloop()
