uint16_t cw_dot_duration = CW_DOT_DURATION;

/*
Process the string message
@param str String to process
*/
void cw_string_proc(String str) {
  for (uint8_t j = 0; j < str.length(); j++)
    cw_char_proc(str[j]);
}

/*
Process a character
transform it to CW symbol
*/
void cw_char_proc(char m) {
  String s;

  if (m == ' ') {                                      // Pause between words
    word_space();
    return;
  }

  if (m > 96)                                          // ACSII, case change a-z to A-Z
    if (m < 123)
      m -= 32;

  switch (m) {                                         // Morse
    case 'A': s = ".-$";     break;
    case 'B': s = "-...$";   break;
    case 'C': s = "-.-.$";   break;
    case 'D': s = "-..$";    break;
    case 'E': s = ".$";      break;
    case 'F': s = "..-.$";   break;
    case 'G': s = "--.$";    break;
    case 'H': s = "....$";   break;
    case 'I': s = "..$";     break;
    case 'J': s = ".---$";   break;
    case 'K': s = "-.-$";    break;
    case 'L': s = ".-..$";   break;
    case 'M': s = "--$";     break;
    case 'N': s = "-.$";     break;
    case 'O': s = "---$";    break;
    case 'P': s = ".--.$";   break;
    case 'Q': s = "--.-$";   break;
    case 'R': s = ".-.$";    break;
    case 'S': s = "...$";    break;
    case 'T': s = "-$";      break;
    case 'U': s = "..-$";    break;
    case 'V': s = "...-$";   break;
    case 'W': s = ".--$";    break;
    case 'X': s = "-..-$";   break;
    case 'Y': s = "-.--$";   break;
    case 'Z': s = "--..$";   break;

    case '1': s = ".----$";  break;
    case '2': s = "..---$";  break;
    case '3': s = "...--$";  break;
    case '4': s = "....-$";  break;
    case '5': s = ".....$";  break;
    case '6': s = "-....$";  break;
    case '7': s = "--...$";  break;
    case '8': s = "---..$";  break;
    case '9': s = "----.$";  break;
    case '0': s = "-----$";  break;

    case '?': s = "..--..$"; break;
    case '=': s = "-...-$"; break;
    case ',': s = "--..--$"; break;
    case '/': s = "-..-.$";  break;
  }

  for (uint8_t i = 0; i < 7; i++) {
    switch (s[i]) {
      case '.': di();  break;
      case '-': dah();  break;
      case '$': char_space(); return;
    }
  }
}

/*
Sending a dit
*/
void di() {
  cw(true);
  delay(cw_dot_duration);

  cw(false);
  delay(cw_dot_duration);
}

/*
Sending a dah
*/
void dah() {
  cw(true);
  delay(3 * cw_dot_duration);

  cw(false);
  delay(cw_dot_duration);
}

/*
Handle character space

3x, pause between letters
1x from end of character + 2x from the beginning of new character
*/
void char_space() {
  delay(2 * cw_dot_duration);
}
/*
Handle word space

7x, pause between words
1x from end of the word + 6x from the beginning of new word
*/
void word_space() {
  delay(6 * cw_dot_duration);
}

/*
Keys the sender up
Turn on the LED
*/
void cw(bool state) {                                  // TX-CW, TX-LED, 750 Hz sound
  if (state) {
    si5351.output_enable(SI5351_CLK0, 1);
    digitalWrite(TX_LED_PIN, HIGH);
  }
  else {
    si5351.output_enable(SI5351_CLK0, 0);
    digitalWrite(TX_LED_PIN, LOW);
  }
}