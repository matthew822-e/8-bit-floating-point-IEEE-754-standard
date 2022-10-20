/* Only Edit This File
 * ------------------
 *  Name:Matthew Emo 
 *  GNumber:G01284640
 */

#include <stdio.h>
#include <stdlib.h>
#include "common_structs.h"
#include "common_functions.h"
#include "kifp.h"

// Feel free to add many Helper Functions, Consts, and Definitions!
kifp_t setKifp(Number_t *number, int Exp);
// ----------Public API Functions (write these!)-------------------

// toKifp - Converts a Number Struct (whole and fraction parts) into a KIFP Value
// number is managed by zuon, DO NOT FREE number.
// Return the KIFP Value on Success or -1 on Failure.
kifp_t toKifp(Number_t *number) {
	int removed_bit = 0;
	int E = 0;
	int bias = 3;
		
	if(number == NULL){return -1;}//incase of error input
		
	//check all the flags before processing the Float
	//handle a neg value
	if(number->whole < 0){
		number->is_negative = 1;
		number->whole = (number->whole & 0x7FFFFFFF);	
	}//procced as if it is a positive number
	if(number->is_infinity && number->is_nan){
		number->fraction = 0x80000000;
		return setKifp(number, 0x00000007); // incoding for NAN
	}else if(number->is_infinity){
		number->fraction = 0;
		return setKifp(number , 0x00000007);// set for -inf or inf
	}else if(number->is_nan){
		number->fraction = 0x80000000;
		return setKifp(number, 0x000000007); // incoding for NAN
	}


	if(number->whole == 0){//Need to Shift decimal to the right so get the desired form
		
		while(number->whole!=1){
			if(number->fraction == 0){ //stop the loop since whole can never get to = 1
				return setKifp(number, 0); //Must be equal to 0 since whole = 0 and fract = 0
			}
			removed_bit = number->fraction & 0x80000000;
			number->fraction = ((number->fraction) << 1); // move the decimal to the right or move the MSB of fract the the LSB of whole
			if(removed_bit!=0){// fract removed a 1 bit 
				number->whole = 1; //since we know that the whole is = 0 in this if statment we can simply set it equal to 1 then a 1 bit is removed from the fract
			}
			--E;// keeping count of the shift and direction
			if(E+bias == 0){//must be denormalied number 
				E = -2;
				//since you want E = -2;
				//must shift the right once to get -3 + 1 = -2
				
			       	if(number->fraction >= 0 && number->whole != 1){//not neg and whole has no number to be added to the frac after shifting
					number->fraction = ((number->fraction) >> 1);
				}else if(number->fraction >= 0 && number->whole == 1){// not neg and whole had a number to add to the fract after shifting
					number->fraction = ((number->fraction) >> 1);
					number->fraction = number->fraction | 0x80000000;// adding the 1 from the whole after shifting
				}else if(number->fraction <= -1 && number->whole != 1){// is neg and the whole does not have a 1 to be carryed over the the frac
					number->fraction = ((number->fraction) >> 1);
					number->fraction = ((number->fraction) & 0x7FFFFFFF);//since all shifts with neg int result in a 1 fill. we make the MSB 0	
				}else if(number->fraction <= -1 && number->whole == 1){//is neg and whole has a 1 bit to add to the frac 
					number->fraction = ((number->fraction) >> 1);//will auto fill with a 1 when shifted thats what we want to happen
				}	
				return setKifp(number, 0);
			
			}
			if(number->whole == 1){// must be normalized and in the right form to return
				return setKifp(number, E + bias);
			}
		}
	}else if(number->whole>=1){//need to shift the deciaml to the left to get the whole number to 0
		E = 0;
		while(number->whole!=1){
			
			removed_bit = (number->whole & 0x00000001);
			number->whole = ((number->whole) >> 1);
			if(removed_bit == 0 && number->fraction >= 0){// simply case just need to shift to the right since not neg and 0 removed
				number->fraction = ((number->fraction) >> 1);
				++E;
			}else if(removed_bit == 0 && number->fraction <=-1){//must toggle the new bit with a 0
				number->fraction = ((number->fraction) >> 1);
				number->fraction = (number->fraction & 0x7FFFFFFF);
				++E;
			}else if(removed_bit == 1 && number->fraction >= 0){//set the new bit shifted to 1 
				number->fraction = ((number->fraction) >> 1);
				number->fraction = (number->fraction | 0x80000000);
				++E;
			}else if(removed_bit == 1 && number->fraction <= -1){
				number->fraction = ((number->fraction) >> 1);
				++E;
			}
			//check for INF number you can stop exection and return inf
			if(E+bias == 7){
				number->fraction = 0;
				return setKifp(number, 0x00000007);
			}
		}
		// if you are here you must have a valid number in the correct formula  
		return setKifp(number, E + bias);
	}	

	return -1;  // error
}

// toNumber - Converts a KIFP Value into a Number Struct (whole and fraction parts)
// number is managed by zuon, DO NOT FREE or re-Allocate number. (It is already allocated)
// Return 0 on Success or -1 on Failure.
int toNumber(Number_t *number, kifp_t value) {
 	
	if(number == NULL || value >= 0x00000200 || value <0){//error check
		return -1;
	}
	int removed_bit = 0; //used for shifting
	int E = (((value & 0x000000e0) >> 5) - 3); //E value for kifp_t incoding
	if(value & 0x00000100){//number is negative
		number->is_negative = 1;
	}
	// fill in the number feilds with non shifted values
	if(E == -3){//must be denormalied
		number->whole = 0;
		number->fraction = value << 27;
		number->is_infinity = 0;//error handling
		number->is_nan = 0;//error handling
		E = -2;

	}else if(E > 3){//special case
		if(value << 27 == 0){//number must be inf
			number->is_infinity = 1;
			number->is_nan = 0;
			return 0;
		}else{ //must be nan
			number->is_infinity = 0;
			number->is_nan = 1;
			return 0;
		}	
		
	}else{//must be normalized 
		number->whole = 1;
		number->fraction = value << 27;
	}
	// get E = 0 and return the method
	if(E < 0){//right shift
		while(E!=0){

                        removed_bit = (number->whole & 0x00000001);
                        number->whole = ((number->whole) >> 1);
                        if(removed_bit == 0 && number->fraction >= 0){// simply case just need to shift to the right since not neg and 0 removed
                                number->fraction = ((number->fraction) >> 1);
                                ++E;
                        }else if(removed_bit == 0 && number->fraction <=-1){//must toggle the new bit with a 0
                                number->fraction = ((number->fraction) >> 1);
                                number->fraction = (number->fraction & 0x7FFFFFFF);
                                ++E;
                        }else if(removed_bit == 1 && number->fraction >= 0){//set the new bit shifted to 1 
                                number->fraction = ((number->fraction) >> 1);
                                number->fraction = (number->fraction | 0x80000000);
                                ++E;
                        }else if(removed_bit == 1 && number->fraction <= -1){
                                number->fraction = ((number->fraction) >> 1);
                                ++E;
                        }
                        
                }

	}else if(E > 0){//left shift
		while(E != 0){
			removed_bit = number->fraction & 0x80000000;
                        number->fraction = ((number->fraction) << 1); // move the decimal to the right or move the MSB of fract the the LSB of whole
                        number->whole = ((number->whole) << 1);
			if(removed_bit!=0){// fract removed a 1 bit 
                                number->whole = number->whole + 1; // set the LSB in whole to 1 since a 1 was shifted out of the fraction 
                        }else{
				//add 0 bit to the whole 
			}
                        --E;// keeping count of the shift and direction
		}	
	}

       	return 0;  // Replace this Line with your Code
}

// mulKIFP - Multiplies two KIFP Values together using the Techniques from Class.
// - To get credit, you must work with S, M, and E components.
// - You are allowed to shift/adjust M and E to multiply whole numbers.
// Return the resulting KIFP Value on Success or -1 on Failure.
kifp_t mulKifp(kifp_t val1, kifp_t val2) {
  
	if(val1 >= 0x00000200 || val2 >= 0x00000200 || val1 < 0 || val2 < 0){
                return -1;
        }
	
	kifp_t val1_E = (((val1 & 0x000000e0) >> 5) - 3); //E value for kifp_t incoding
        kifp_t val1_S = (val1 & 0x00000100); //sign of the number
        kifp_t val1_W = 0x00000001; // whole of the number
        kifp_t val1_F = val1 << 27; // fraction of the number

        kifp_t val2_E = (((val2 & 0x000000e0) >> 5) - 3); //E value for kifp_t incoding
        kifp_t val2_S = (val2 & 0x00000100); //sign of the number
        kifp_t val2_W = 0x00000001; // whole of the number
        kifp_t val2_F = val2 << 27; // fraction of the number
	
	kifp_t return_sign = val1_S ^ val2_S;
	kifp_t return_E = val1_E + val2_E;
	kifp_t return_W = 0;
	kifp_t return_F = 0;
	kifp_t kifp_t_return = 0;
	int removed_bit;
	if(val1_E >= 4 || val2_E >= 4){//handle special cases 
                printf("ENTERED\n");
		if((val1_E >= 4 && val1_F != 0) || (val2_E >= 4 && val2_F != 0)){
                        return_W = 0x000000E1;
                        return return_W;
                }else if((val1_E >= 4 && val1_F == 0) || (val2_E >= 4 && val2_F == 0)){
			if(val1_E == 0 || val2_E == 0){
				return_W = 0x000000E1;
                        	return return_W;//return NaN
			}else{
				if(return_sign >= 1){
					return_W = 0x000001E0;
				}else{
					return_W = 0x000000E0;
				}
				return return_W;
			}

		}

        }
	
	if(val1 == 0 || val2 == 0){//if any of the numbers is 0 need to return 0
		val1 = 0;
		return val1;
	}
	
	//check for overflow
	if(return_E > 3){
		if(return_sign >= 1){
                    return_W = 0x000001E0;
                    return return_W;
		}else{
                    return_W = 0x000000E0;
		    return return_W;
                }
	}
	
	kifp_t val1_M = val1 & 0x0000001F;//clear all bits other then Fract part
	kifp_t val2_M = val2 & 0x00000001F;//clear all bits other then Fract part
	
	
	if(val1_E != -3){//normalized so add the whole else keep 0 for denormalized 
		val1_M = val1_M | 0x00000020;//adding the whole to this number
	}
	if(val2_E != -3){
		val2_M = val2_M | 0x00000020;//adding the whole to this number
	}

	while(val2_M != 0){
        	if (val2_M & 0x00000001){
         	//add whole 1 to total
         	return_W = return_W + val1_M;
        	}
        val2_M = val2_M >> 1;
        val1_M = val1_M << 1;
    	}
	
	//find where the decimal should be shifting the val to the right 10 times
	for(int i = 0; i<10; i++){//right shift 10 times
		//shift right and increament val1_E
                        
			removed_bit = (return_W & 0x00000001);
                        return_W = (return_W >> 1);
                        if(removed_bit == 0 && return_F >= 0){// simply case just need to shift to the right since not neg and 0 removed
                                return_F = ((return_F) >> 1);
                                
                        }else if(removed_bit == 0 && return_F <=-1){//must toggle the new bit with a 0
                                return_F = ((return_F) >> 1);
                                return_F = (return_F & 0x7FFFFFFF);
                                
                        }else if(removed_bit == 1 && return_F >= 0){//set the new bit shifted to 1 
                                return_F = ((return_F) >> 1);
                                return_F = (return_F | 0x80000000);
                                
                        }else if(removed_bit == 1 && return_F <= -1){
                                return_F = ((return_F) >> 1);
                        }

	}
	printf("result w:%x f:%x\n", return_W, return_F);
	
	// should be in normal form
	

	//incode the result and return the value
        
        if(return_sign > 0){//sets the sign of the result 
                kifp_t_return = (kifp_t_return | 0x00000100);
        }
        kifp_t exp = ((return_E + 3) << 5); //shift the exp so the bits are in the same position they are in Kifp_t
        kifp_t_return = (kifp_t_return | exp);// copy the exp in 
        return_F = ((return_F) >> 1);// get the fract one spot over so we can clear the sign bit
        return_F = ((return_F) & 0x7FFFFFFF);// makes sure the the sign bit is 0 so you shift right you dont fill with a 1 bit on the left
        return_F = ((return_F) >> 26); // puts fract in the position to be copied into kifp_t
        kifp_t_return = (kifp_t_return | return_F);
        return kifp_t_return;	
}

// addKIFP - Adds two KIFP Values together using the Addition Techniques from Class.
// - To get credit, you must work with S, M, and E components.
// - You are allowed to shift/adjust M and E as needed.
// Return the resulting KIFP Value on Success or -1 on Failure.
kifp_t addKifp(kifp_t val1, kifp_t val2) {
	if(val1 >= 0x00000200 || val2 >= 0x00000200 || val1 < 0 || val2 < 0){
		return -1;
	}
	kifp_t val1_E = (((val1 & 0x000000e0) >> 5) - 3); //E value for kifp_t incoding 
	kifp_t val1_S = (val1 & 0x00000100); //sign of the number 
	kifp_t val1_W = 0x00000001; // whole of the number
	kifp_t val1_F = val1 << 27; // fraction of the number
	
	kifp_t val2_E = (((val2 & 0x000000e0) >> 5) - 3); //E value for kifp_t incoding 
        kifp_t val2_S = (val2 & 0x00000100); //sign of the number 
        kifp_t val2_W = 0x00000001; // whole of the number
        kifp_t val2_F = val2 << 27; // fraction of the number
	
	kifp_t return_sign = 0;
	kifp_t return_whole = 0;
	kifp_t return_fraction = 0;
	kifp_t return_exp = 0;
	kifp_t kifp_t_return = 0;
	
	kifp_t removed_bit;
	
	//print for debuging 
	
	if((val1 & 0x00000100) && (val2 & 0x00000100)){//both are negative add like normal set the return S bit to neg
                return_sign = 1;
        }else if(val1 > val2 && val1_S != 0){//larger number is neg must be a neg result
                return_sign = 1;
        }else if(val1 < val2 && val2_S != 0){
		return_sign = 1;
	}

	if(val1_E == 4 || val2_E == 4){//special cases need to be handled 
		if(val1_F != 0 || val2_F != 0){//nan
			kifp_t_return = 0x000000E1;
			return kifp_t_return;
		}else {//must be inf
		       if(return_sign == 1){
				kifp_t_return = 0x000001E0;
                        	return kifp_t_return;
			}else{
				kifp_t_return = 0x000000E0;
                        	return kifp_t_return;
			}		
		}
	}
	
	// if input is negative need to negate the floating point before adding
        
        if((val1_S != 0) && (val2_S != 0)){//both are negative add like normal set the return S bit to neg
                //do nothing to the numbers
                //add them normally and at the end the sign bit should be set to a negative

        }else if(val1_S != 0){//since val1 is negative we call subtraction method
                        //reset the sign bit so its in the form val2 - val1
                        val1 = val1 & 0xFFFFFEFF;
                        return subKifp(val2, val1);

        }else if(val2_S != 0){
                //reset the sign bit so it is in the form val1 - val2
                val2 = val2 & 0xFFFFFEFF;
                return subKifp(val1, val2);
        }



	//check for denormalized cases and handle
        if(val1_E == -3 || val2_E == -3){//must be denormalied
                if(val1_E == -3){
			val1_W = 0x00000000;;
               		val1_E = -2;
		}
		if(val2_E == -3){
			val2_W = 0x00000000;
                        val2_E = -2;
		}
	}	
	

	

	//make the E match up 
	//always be right shifts to the number
	if(val1_E < val2_E){//shift val1 to match val2
	
		while(val1_E != val2_E){
			//shift right and increament val1_E
			removed_bit = (val1_W & 0x00000001);
                        val1_W = (val1_W >> 1);
                        if(removed_bit == 0 && val1_F >= 0){// simply case just need to shift to the right since not neg and 0 removed
                                val1_F = ((val1_F) >> 1);
                                ++val1_E;
                        }else if(removed_bit == 0 && val1_F <=-1){//must toggle the new bit with a 0
                                val1_F = ((val1_F) >> 1);
                                val1_F = (val1_F & 0x7FFFFFFF);
                                ++val1_E;
                        }else if(removed_bit == 1 && val1_F >= 0){//set the new bit shifted to 1 
                                val1_F = ((val1_F) >> 1);
                                val1_F = (val1_F | 0x80000000);
                                ++val1_E;
                        }else if(removed_bit == 1 && val1_F <= -1){
                                val1_F = ((val1_F) >> 1);
                                ++val1_E;
                        }

		}
		
	}else if(val1_E > val2_E){//shift val2 to match val1
                while(val2_E != val1_E){
                        //shift right and increament val1_E
                        removed_bit = (val2_W & 0x00000001);
                        val2_W = (val2_W >> 1);
                        if(removed_bit == 0 && val2_F >= 0){// simply case just need to shift to the right since not neg and 0 removed
                                val2_F = ((val2_F) >> 1);
                                ++val2_E;
                        }else if(removed_bit == 0 && val2_F <=-1){//must toggle the new bit with a 0
                                val2_F = ((val2_F) >> 1);
                                val2_F = (val2_F & 0x7FFFFFFF);
                                ++val2_E;
                        }else if(removed_bit == 1 && val2_F >= 0){//set the new bit shifted to 1 
                                val2_F = ((val2_F) >> 1);
                                val2_F = (val2_F | 0x80000000);
                                ++val2_E;
                        }else if(removed_bit == 1 && val2_F <= -1){
                                val2_F = ((val2_F) >> 1);
                                ++val2_E;
                        }

                }

	}
	

	// add the numbers 
	// at this point the E will match
	// first add the fractions 
	val1_F = (val1_F >> 1);
        val2_F = (val2_F >> 1);
        val1_F = val1_F & 0x7FFFFFFF;
        val2_F = val2_F & 0x7FFFFFFF;

        return_fraction = val1_F + val2_F;
       
        removed_bit = return_fraction & 0x80000000;
        return_fraction = (return_fraction << 1); // move the decimal to the right or move the MSB of fract the the LSB of whole
     
        if(removed_bit!=0){// fract removed a 1 bit 
                return_whole ++;
        }
        //next add the whole 
        return_whole = return_whole + val1_W + val2_W;
	


	//normalize the results
	if(return_whole == 0){//Need to Shift decimal to the right to get the desired form
		
                while(return_whole!=1){
                        if(return_fraction == 0){ //stop the loop since whole can never get to = 1
                                //return setKifp(number, 0); //Must be equal to 0 since whole = 0 and fract = 0
                        }
                        removed_bit = return_fraction & 0x80000000;
                        return_fraction = ((return_fraction) << 1); // move the decimal to the right or move the MSB of fract the the LSB of whole
                        
			if(removed_bit!=0){// fract removed a 1 bit 
                                return_whole = 1; //since we know that the whole is = 0 in this if statment we can simply set it equal to 1 then a 1 bit is removed from the fract
                        }
                        --val1_E;// keeping count of the shift and direction
                        if(val1_E + 3 == 0){//must be denormalied number 
                         
                                //since you want E = -2;
                                //must shift the right once to get -3 + 1 = -2
				if(return_fraction >= 0 && return_whole != 1){//not neg and whole has no number to be added to the frac after shifting
                                        return_fraction = ((return_fraction) >> 1);
                                }else if(return_fraction >= 0 && return_whole == 1){// not neg and whole had a number to add to the fract after shifting
                                        return_fraction = ((return_fraction) >> 1);
                                        return_fraction = return_fraction | 0x80000000;// adding the 1 from the whole after shifting
                                }else if(return_fraction <= -1 && return_whole != 1){// is neg and the whole does not have a 1 to be carryed over the the frac
                                        return_fraction = ((return_fraction) >> 1);
                                        return_fraction = ((return_fraction) & 0x7FFFFFFF);//since all shifts with neg int result in a 1 fill. we make the MSB 0      
                                }else if(return_fraction <= -1 && return_whole == 1){//is neg and whole has a 1 bit to add to the frac 
                                        return_fraction = ((return_fraction) >> 1);//will auto fill with a 1 when shifted thats what we want to happen
                                }
                                break;

                        }
                }
        }else if(return_whole > 1){//need to Shift the decimal to the left to get desired form
		while(return_whole!=1){
                        removed_bit = (return_whole & 0x00000001);
                        return_whole = ((return_whole) >> 1);
                        if(removed_bit == 0 && return_fraction >= 0){// simply case just need to shift to the right since not neg and 0 removed
                                return_fraction = ((return_fraction) >> 1);
                                ++val1_E;
                        }else if(removed_bit == 0 && return_fraction <=-1){//must toggle the new bit with a 0
                                return_fraction = ((return_fraction) >> 1);
                                return_fraction = (return_fraction & 0x7FFFFFFF);
                                ++val1_E;
                        }else if(removed_bit == 1 && return_fraction >= 0){//set the new bit shifted to 1 
                                return_fraction = ((return_fraction) >> 1);
                                return_fraction = (return_fraction | 0x80000000);
                                ++val1_E;
                        }else if(removed_bit == 1 && return_fraction <= -1){
                                return_fraction = ((return_fraction) >> 1);
                                ++val1_E;
                        }
                        //check for INF number you can stop exection and return inf
                        if(val1_E + 3 >= 7){
                                return_fraction = 0;
				break;
				
                        }
		}

	}
	//incode the result and return the value
	kifp_t_return = 0;
	if(return_sign != 0){//sets the sign of the result 
		kifp_t_return = (kifp_t_return | 0x00000100);
	}	
        kifp_t exp = ((val1_E + 3) << 5); //shift the exp so the bits are in the same position they are in Kifp_t
        kifp_t_return = (kifp_t_return | exp);// copy the exp in 
	return_fraction = ((return_fraction) >> 1);// get the fract one spot over so we can clear the sign bit
        return_fraction = ((return_fraction) & 0x7FFFFFFF);// makes sure the the sign bit is 0 so you shift right you dont fill with a 1 bit on the left
        return_fraction = ((return_fraction) >> 26); // puts fract in the position to be copied into kifp_t
	kifp_t_return = (kifp_t_return | return_fraction);
	return kifp_t_return;
}

// subKIFP - Subtracts two KIFP Values together using the Addition Techniques from Class.
// - To get credit, you must work with S, M, and E components.
// - You are allowed to shift/adjust M and E as needed.
// Return the resulting KIFP Value on Success or -1 on Failure.
kifp_t subKifp(kifp_t val1, kifp_t val2) {
  	if(val1 >= 0x00000200 || val2 >= 0x00000200 || val1 < 0 || val2 < 0){
                return -1;
        }
	
        kifp_t val1_E = (((val1 & 0x000000e0) >> 5) - 3); //E value for kifp_t incoding 
        kifp_t val1_S = (val1 & 0x00000100); //sign of the number 
        kifp_t val1_W = 0x00000001; // whole of the number
        kifp_t val1_F = val1 << 27; // fraction of the number

        kifp_t val2_E = (((val2 & 0x000000e0) >> 5) - 3); //E value for kifp_t incoding 
        kifp_t val2_S = (val2 & 0x00000100); //sign of the number 
        kifp_t val2_W = 0x00000001; // whole of the number
        kifp_t val2_F = val2 << 27; // fraction of the number

        kifp_t return_sign = 0;
        kifp_t return_whole = 0;
        kifp_t return_fraction = 0;
        kifp_t return_exp = 0;
        kifp_t kifp_t_return = 0;

        kifp_t removed_bit = 0;

          
	if((val1 & 0x00000100) && (val2 & 0x00000100)){//both are negative add like normal set the return S bit to neg
                return_sign = 1;
        }else if(val1 > val2){//larger number is neg must be a neg result
                return_sign = 0;
        }else if(val1 < val2){
                return_sign = 1;
        }

        if(val1_E == 4 || val2_E == 4){//special cases need to be handled 
                if(val1_F != 0 || val2_F != 0){//nan
                        kifp_t_return = 0x000000E1;
                        return kifp_t_return;
                }else {//must be inf
                       if(return_sign == 1){
                                kifp_t_return = 0x000001E0;
                                return kifp_t_return;
                        }else{
                                kifp_t_return = 0x000000E0;
                                return kifp_t_return;
                        }
                }
        }

	
	
	if(val1 == val2){//return 0 since anything - it's self is 0
                kifp_t_return = 0;
                return kifp_t_return;
        }

        // if input is negative need to negate the floating point before adding
        if((val1_S != 0) && (val2_S != 0)){//both are negative add like normal set the return S bit to neg
                //do nothing to the numbers
                //add them normally and at the end the sign bit should be set to a negative
		val1 = val1 & 0xFFFFFEFF;
		val2 = val2 & 0xFFFFFEFF;//reset the sign bit
		//(-x - -y ) = y - x
		return subKifp(val2, val1);
	
        }else if(val1_S != 0){//since val1 is negative we call subtraction method
                        //set sign bit so its - for va2
                        val2 = val2 | 0x00000100;
                       	return addKifp(val1, val2);

        }else if(val2_S != 0){
                //reset the sign bit so it is in the form val1 - val2
                val2 = val2 & 0xFFFFFEFF; // since (x - -y) == x + y 
                return addKifp(val1, val2); //we use add in this case
        }else if(val2 > val1){//know both numbers must be poitive
		kifp_t_return = subKifp(val2, val1);// send it in as if it was the inverse
		kifp_t_return = (kifp_t_return | 0x00000100);// universe the number
		return kifp_t_return;
	}

	
	
        //check for denormalized cases and handle
        if(val1_E == -3 || val2_E == -3){//must be denormalied
                if(val1_E == -3){
                        val1_W = 0x00000000;;
                        val1_E = -2;
                }
                if(val2_E == -3){
                        val2_W = 0x00000000;
                        val2_E = -2;
                }
        }
	

	//make the E match up 
        //always be right shifts to the number
        if(val1_E < val2_E){//shift val1 to match val2
                while(val1_E != val2_E){
                        //shift right and increament val1_E
                        removed_bit = (val1_W & 0x00000001);
                        val1_W = (val1_W >> 1);
                        if(removed_bit == 0 && val1_F >= 0){// simply case just need to shift to the right since not neg and 0 removed
                                val1_F = ((val1_F) >> 1);
                                ++val1_E;
                        }else if(removed_bit == 0 && val1_F <=-1){//must toggle the new bit with a 0
                                val1_F = ((val1_F) >> 1);
                                val1_F = (val1_F & 0x7FFFFFFF);
                                ++val1_E;
                        }else if(removed_bit == 1 && val1_F >= 0){//set the new bit shifted to 1 
                                val1_F = ((val1_F) >> 1);
                                val1_F = (val1_F | 0x80000000);
                                ++val1_E;
                        }else if(removed_bit == 1 && val1_F <= -1){
                                val1_F = ((val1_F) >> 1);
                                ++val1_E;
                        }

                }

        }else if(val1_E > val2_E){//shift val2 to match val1
                while(val2_E != val1_E){
                        //shift right and increament val1_E
                        removed_bit = (val2_W & 0x00000001);
                        val2_W = (val2_W >> 1);
                        if(removed_bit == 0 && val2_F >= 0){// simply case just need to shift to the right since not neg and 0 removed
                                val2_F = ((val2_F) >> 1);
                                ++val2_E;
                        }else if(removed_bit == 0 && val2_F <=-1){//must toggle the new bit with a 0
                                val2_F = ((val2_F) >> 1);
                                val2_F = (val2_F & 0x7FFFFFFF);
                                ++val2_E;
                        }else if(removed_bit == 1 && val2_F >= 0){//set the new bit shifted to 1 
                                val2_F = ((val2_F) >> 1);
                                val2_F = (val2_F | 0x80000000);
                                ++val2_E;
                        }else if(removed_bit == 1 && val2_F <= -1){
                                val2_F = ((val2_F) >> 1);
                                ++val2_E;
                        }

                }

        }
        printf("THE RESULT AFTER SHIFTING TO MATCH IS whole: %x fraction: %x\n", val1_W, val1_F);
	
	// add the numbers 
        // at this point the E will match
        // first add the fractions 
        printf("VALUES BEFORE ADDING");
        printf("val1: E=%x W=%x F=%x S=%x \n", val1_E, val1_W, val1_F, val1_S);
        printf("val2: E=%x W=%x F=%x S=%x \n", val2_E, val2_W, val2_F, val2_S);
        //Negtion of the number 
	if(val2_W == 1){
		val2_W = 0;
	}else{
		val2_W = 1;
	}
	val2_F = ~val2_F + 1;
	val1_F = (val1_F >> 1);
        val2_F = (val2_F >> 1);
        val1_F = val1_F & 0x7FFFFFFF;
        val2_F = val2_F & 0x7FFFFFFF;
	
        return_fraction = val1_F + val2_F;

        removed_bit = return_fraction & 0x80000000;
        return_fraction = (return_fraction << 1); // move the decimal to the right or move the MSB of fract the the LSB of whole

        if(removed_bit!=0){// fract removed a 1 bit 
                return_whole++;
        }
        //next add the whole 
	if(val1_W == 1 && val2_W == 1){
		return_whole = 0;
	}else{
		return_whole = 0;
	}
       



	 //normalize the results
        if(return_whole == 0){//Need to Shift decimal to the right to get the desired form

                while(return_whole!=1){
                        if(return_fraction == 0){ //stop the loop since whole can never get to = 1
                                //return setKifp(number, 0); //Must be equal to 0 since whole = 0 and fract = 0
                        }
                        removed_bit = return_fraction & 0x80000000;
                        return_fraction = ((return_fraction) << 1); // move the decimal to the right or move the MSB of fract the the LSB of whole

                        if(removed_bit!=0){// fract removed a 1 bit 
                                return_whole = 1; //since we know that the whole is = 0 in this if statment we can simply set it equal to 1 then a 1 bit is removed from the fract
                        }
                        --val1_E;// keeping count of the shift and direction
                        if(val1_E + 3 == 0){//must be denormalied number 
                                
                                //shift once to the right and return that number
                                
                                if(return_fraction >= 0 && return_whole != 1){//not neg and whole has no number to be added to the frac after shifting
                                        return_fraction = ((return_fraction) >> 1);
                                }else if(return_fraction >= 0 && return_whole == 1){// not neg and whole had a number to add to the fract after shifting
                                        return_fraction = ((return_fraction) >> 1);
                                        return_fraction = return_fraction | 0x80000000;// adding the 1 from the whole after shifting
                                }else if(return_fraction <= -1 && return_whole != 1){// is neg and the whole does not have a 1 to be carryed over the the frac
                                        return_fraction = ((return_fraction) >> 1);
                                        return_fraction = ((return_fraction) & 0x7FFFFFFF);//since all shifts with neg int result in a 1 fill. we make the MSB 0      
                                }else if(return_fraction <= -1 && return_whole == 1){//is neg and whole has a 1 bit to add to the frac 
                                        return_fraction = ((return_fraction) >> 1);//will auto fill with a 1 when shifted thats what we want to happen
                                }
                                break;
                                


                        }
                }
        }else if(return_whole > 1){//need to Shift the decimal to the left to get desired form
                while(return_whole!=1){
                        removed_bit = (return_whole & 0x00000001);
                        return_whole = ((return_whole) >> 1);
                        if(removed_bit == 0 && return_fraction >= 0){// simply case just need to shift to the right since not neg and 0 removed
                                return_fraction = ((return_fraction) >> 1);
                                ++val1_E;
                        }else if(removed_bit == 0 && return_fraction <=-1){//must toggle the new bit with a 0
                                return_fraction = ((return_fraction) >> 1);
                                return_fraction = (return_fraction & 0x7FFFFFFF);
                                ++val1_E;
                        }else if(removed_bit == 1 && return_fraction >= 0){//set the new bit shifted to 1 
                                return_fraction = ((return_fraction) >> 1);
                                return_fraction = (return_fraction | 0x80000000);
                                ++val1_E;
                        }else if(removed_bit == 1 && return_fraction <= -1){
                                return_fraction = ((return_fraction) >> 1);
                                ++val1_E;
                        }
                        //check for INF number you can stop exection and return inf
                        if(val1_E + 3 >= 7){
                                return_fraction = 0;
                                break;

                        }
                }

        }
	//incode the result and return the value
        kifp_t_return = 0;
        if(return_sign != 0){//sets the sign of the result 
                kifp_t_return = (kifp_t_return | 0x00000100);
        }

        printf("Fract before return %x\n", kifp_t_return);
        kifp_t exp = ((val1_E + 3) << 5); //shift the exp so the bits are in the same position they are in Kifp_t
        kifp_t_return = (kifp_t_return | exp);// copy the exp in 
        printf("exp %x\n", kifp_t_return);
        return_fraction = ((return_fraction) >> 1);// get the fract one spot over so we can clear the sign bit
        return_fraction = ((return_fraction) & 0x7FFFFFFF);// makes sure the the sign bit is 0 so you shift right you dont fill with a 1 bit on the left
        return_fraction = ((return_fraction) >> 26); // puts fract in the position to be copied into kifp_t
        printf("Fract  %x\n", kifp_t_return);
        kifp_t_return = (kifp_t_return | return_fraction);
        printf("final %x\n", kifp_t_return);
        return kifp_t_return;	
	
}

// negateKIFP - Negates a KIFP Value.
// Return the resulting KIFP Value on Success or -1 on Failure.
kifp_t negateKifp(kifp_t value) { 
	value ^= 0x00000001 << 8;
	return value;  // Replace this Line with your Code
}
kifp_t setKifp(Number_t *number, int exp){
	
	kifp_t kifp_t_return = 0;	
	//set the s bit
	if(number->is_negative == 1){
		kifp_t_return = (kifp_t_return | 0x00000080);
	}//else leave it as a 0
	exp = exp << 5; //shift the exp so the bits are in the same position they are in Kifp_t
	kifp_t_return = (kifp_t_return | exp);// copy the exp in 
	number->fraction = (number->fraction) >> 1;// get the fract one spot over so we can clear the sign bit
	number->fraction = ((number->fraction) & 0x7FFFFFFF);// makes sure the the sign bit is 0 so you shift right you dont fill with a 1 bit on the left
	number->fraction = ((number->fraction) >> 26); // puts fract in the position to be copied into kifp_t
	kifp_t_return = (kifp_t_return | number->fraction);
	return kifp_t_return;
	
}
