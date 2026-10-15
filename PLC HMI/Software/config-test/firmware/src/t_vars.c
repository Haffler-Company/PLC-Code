#include <string.h>
#include "t_vars.h"


 int  buzzer = 180;

bool q1 = 0;
bool q2 = 0;
bool q3 = 0;
bool q4 = 0;
bool q5 = 0;
bool q6 = 0;
bool q7 = 0;
bool q8 = 0;
bool q9 = 0;
bool q10 = 0;

bool f1 = 1;
bool f2 = 1;
bool f3 = 1;
bool f4 = 1;
bool f5 = 1;
bool f6 = 1;
bool f7 = 1;
bool f8 = 1;
bool f9 = 1;
bool f10 = 1;

bool _10 = 1;
bool _11 = 1;
bool _12 = 1;
bool _13 = 1;
bool _14 = 1;
bool _15 = 1;
bool _16 = 1;
bool _17 = 1;

bool send = 0;
bool receive = 0;

bool receive_message = 0;
bool send_message = 0;


bool enableble = 0;
bool changblelabl = 1;

bool anbl = 0; 
bool scanwifi = 1;


bool new =1;
bool rtc_error;
bool touch_error;



char rtc_text[100] = { 0 };
bool rtc_apply;




char time_text[100] = { 0 };
bool pwm_state = 0;




bool ssrloaded = 0;
bool setuploaded = 0;



char rstextsend[100] = { 0 };







extern int32_t get_var_buzzer(){
	return buzzer;
}
extern void set_var_buzzer(int32_t value){
  if (value < 0) value = 0;
  if (value > 255) value = 255;
  buzzer = value;
}



//OUTPUTS
extern bool get_var_q1()
{
	return q1;
}
extern void set_var_q1(bool value)
{
	q1 = value;
}
extern bool get_var_q2()
{
	return q2;
}
extern void set_var_q2(bool value)
{
	q2 = value;
}
extern bool get_var_q3()
{
	return q3;
}
extern void set_var_q3(bool value)
{
	q3 = value;
}
extern bool get_var_q4()
{
	return q4;
}
extern void set_var_q4(bool value)
{
	q4 = value;
}
extern bool get_var_q5()
{
	return q5;
}
extern void set_var_q5(bool value)
{
	q5 = value;
}
extern bool get_var_q6()
{
	return q6;
}
extern void set_var_q6(bool value)
{
	q6 = value;
}
extern bool get_var_q7()
{
	return q7;
}
extern void set_var_q7(bool value)
{
	q7 = value;
}
extern bool get_var_q8()
{
	return q8;
}
extern void set_var_q8(bool value)
{
	q8 = value;
}
extern bool get_var_q9()
{
	return q9;
}
extern void set_var_q9(bool value)
{
	q9 = value;
}
extern bool get_var_q10()
{
	return q10;
}
extern void set_var_q10(bool value)
{
	q10 = value;
}



//BUTTONS
extern bool get_var_f1(){
  return f1;
}
extern void set_var_f1(bool value){
	f1=!f1;
}
extern bool get_var_f2(){
  return f2;

}
extern void set_var_f2(bool value){
	f2=!f2;
}
extern bool get_var_f3(){
  return f3;

}
extern void set_var_f3(bool value){
	f3=!f3;
}
extern bool get_var_f4(){
  return f4;

}
extern void set_var_f4(bool value){
	f4=!f4;
}
extern bool get_var_f5(){
  return f5;
	
}
extern void set_var_f5(bool value){
	f5=!f5;
}
extern bool get_var_f6(){
  return f6;

}
extern void set_var_f6(bool value){
 f6=!f6;
}
extern bool get_var_f7(){
  return f7;

}
extern void set_var_f7(bool value){
	f7=!f7;
}
extern bool get_var_f8(){
  return f8;

}
extern void set_var_f8(bool value){
	f8=!f8;
}
extern bool get_var_f9(){
  return f9;

}
extern void set_var_f9(bool value){

	f9=!f9;
}
extern bool get_var_f10(){
  return f10;

}
extern void set_var_f10(bool value){
	f10=!f10;
}




//INPUTS
bool get_var__10() {
    return _10;
}
void set_var__10(bool value) {
    _10 = value;
}




bool get_var__11() {
    return _11;
}
void set_var__11(bool value) {
    _11 = value;
}



bool get_var__12() {
    return _12;
}
void set_var__12(bool value) {
    _12 = value;
}


bool get_var__13() {
    return _13;
}
void set_var__13(bool value) {
    _13 = value;
}



bool get_var__14() {
    return _14;
}
void set_var__14(bool value) {
    _14 = value;
}




bool get_var__15() {
    return _15;
}
void set_var__15(bool value) {
    _15 = value;
}



bool get_var__16() {
    return _16;
}
void set_var__16(bool value) {
    _16 = value;
}



bool get_var__17() {
    return _17;
}
void set_var__17(bool value) {
    _17 = value;
}








//RS485
extern bool get_var_send(){
   return send;
}
extern void set_var_send(bool value){
   send = value;
}
extern bool get_var_receive(){
   return receive;
}
extern void set_var_receive(bool value){
   receive = value;
}



//GSM
extern bool get_var_receive_message(){
   return false;
}
extern void set_var_receive_message(bool value){

}
extern bool get_var_send_message(){
   return false;
}
extern void set_var_send_message(bool value){

}




//BLUTOOTH
extern bool get_var_enableble(){
   return enableble;
}
extern void set_var_enableble(bool value){
   enableble = !enableble;
  // enableble = value;
}


//BLUTOOTH labl
extern bool get_var_changblelabl(){
    return changblelabl;
}
extern void set_var_changblelabl(bool value){
    changblelabl = !changblelabl;
}






//wifi
extern bool get_var_anbl(){
return anbl;
}
extern void set_var_anbl(bool value){
//anbl = value;
anbl = !anbl;

}


//wifi labl
bool get_var_scanwifi() {
    return scanwifi;
}
void set_var_scanwifi(bool value) {
    scanwifi = !scanwifi;
}






bool get_var_new() {
    return new;
}

void set_var_new(bool value) {
    new = value;
}





bool get_var_touch_error() {
    return touch_error;
}

void set_var_touch_error(bool value) {
    touch_error = value;
}


bool get_var_rtc_error() {
    return rtc_error;
}

void set_var_rtc_error(bool value) {
    rtc_error = value;
}






const char *get_var_rtc_text() {
    return rtc_text;
}

void set_var_rtc_text(const char *value) {
    strncpy(rtc_text, value, sizeof(rtc_text) / sizeof(char));
    rtc_text[sizeof(rtc_text) / sizeof(char) - 1] = 0;
}



bool get_var_rtc_apply() {
    return rtc_apply;
}

void set_var_rtc_apply(bool value) {
    rtc_apply = value;
}







const char *get_var_time_text() {
    return time_text;
}

void set_var_time_text(const char *value) {
    strncpy(time_text, value, sizeof(time_text) / sizeof(char));
    time_text[sizeof(time_text) / sizeof(char) - 1] = 0;
}



bool get_var_pwm_state() {
    return pwm_state;
}

void set_var_pwm_state(bool value) {
    pwm_state = value;
}






bool get_var_setuploaded() {
    return setuploaded;
}

void set_var_setuploaded(bool value) {
    setuploaded = value;
}


bool get_var_ssrloaded() {
    return ssrloaded;
}

void set_var_ssrloaded(bool value) {
    ssrloaded = value;
}





const char *get_var_rstextsend() {
    return rstextsend;
}

void set_var_rstextsend(const char *value) {
    strncpy(rstextsend, value, sizeof(rstextsend) / sizeof(char));
    rstextsend[sizeof(rstextsend) / sizeof(char) - 1] = 0;
}