#ifndef TRACKSENSE_HPP
#define TRACKSENSE_HPP

/**
 * @class Carro
 * @brief Representa um carro no sistema.
 * @details
 * A classe `Carro` gerencia informações e comportamentos de um veículo,
 * incluindo sua marca, modelo, velocidade atual e métodos para acelerar,
 * frear e obter dados. 
 *
 * Exemplo de uso:
 * @code
 * Carro c("Toyota", "Corolla");
 * c.acelerar(20);
 * std::cout << c.getVelocidade() << std::endl;
 * @endcode
 */
class Carro {
public:
    /**
     * @brief Construtor do carro.
     * @param marca Marca do carro.
     * @param modelo Modelo do carro.
     */
    Carro(const std::string& marca, const std::string& modelo);

    /**
     * @brief Acelera o carro.
     * @details
     * Incrementa a velocidade atual pelo valor informado.
     * Se o valor for negativo, ele é ignorado.
     * @param valor Valor em km/h para aumentar a velocidade.
     */
    void acelerar(int valor);

    /**
     * @brief Reduz a velocidade do carro.
     * @param valor Valor em km/h para reduzir a velocidade.
     */
    void frear(int valor);

    /**
     * @brief Obtém a velocidade atual.
     * @return Velocidade em km/h.
     */
    int getVelocidade() const;

private:
    std::string marca;   ///< Marca do carro.
    std::string modelo;  ///< Modelo do carro.
    int velocidade;      ///< Velocidade atual do carro em km/h.

    /**
     * @brief Método auxiliar para validar a velocidade.
     * @param v Velocidade a validar.
     * @return true se for válida, false caso contrário.
     */
    bool validarVelocidade(int v) const;
};
















#endif // tRACK