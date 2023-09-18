# vumeter
Un vumetro para conectarlo a la PC mediante bluetooth

# Conexiones: 

los pines 3,4,5,6 del atmega, van a los 4 bits mas significativos (4,5,6,7) del lcd 

pin LCD_PORT1         PORTB        /**< port for the LCD lines   */

pin LCD_PORT2         PORTB


pin LCD_DATA0_PORT   LCD_PORT2     /**< port for 4bit data bit 0 */

pin LCD_DATA1_PORT   LCD_PORT2     /**< port for 4bit data bit 1 */

pin LCD_DATA2_PORT   LCD_PORT2     /**< port for 4bit data bit 2 */

pin LCD_DATA3_PORT   LCD_PORT2     /**< port for 4bit data bit 3 */

pin LCD_DATA0_PIN    3            /**< pin for 4bit data bit 0  */
pin LCD_DATA1_PIN    4            /**< pin for 4bit data bit 1  */
pin LCD_DATA2_PIN    5           /**< pin for 4bit data bit 2  */
pin LCD_DATA3_PIN    6            /**< pin for 4bit data bit 3  */
pin LCD_RS_PORT      LCD_PORT1     /**< port for RS line         */
pin LCD_RS_PIN       0            /**< pin  for RS line         */
pin LCD_RW_PORT      LCD_PORT1     /**< port for RW line         */
pin LCD_RW_PIN       1            /**< pin  for RW line         */
pin LCD_E_PORT       LCD_PORT1     /**< port for Enable line     */
pin LCD_E_PIN        2            /**< pin  for Enable line     */
