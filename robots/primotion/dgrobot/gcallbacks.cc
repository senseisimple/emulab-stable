/* Garcia robot callback class methods
 *
 * Dan Flickinger
 *
 * 2004/11/16
 * 2004/11/19
 *
 */
 
#include "gcallbacks.h"

CallbackComplete::CallbackComplete(acpObject *b, grobot *g, cb_type_t cbt) {
  behavior = b;
  pgrobot = g;

  blast_status = 0x000;
  
  // get ID of current behavior
  blast_id = behavior->getNamedValue("unique-id")->getIntVal();

  this->cbt = cbt;
  
  //strcpy(lastMSG, "no messages");
  // cout << "Created Callback" << endl;
}



CallbackComplete::~CallbackComplete() {
  // cout << "Deleting callback" << endl;
  
  // send back the status message
  
  // for now, dump callback messages to stdout:
  //cout << "CB: " << blast_status << endl; 
  

}


aErr CallbackComplete::call() {
  // call completion callback
  
  blast_status = behavior->getNamedValue("completion-status")->getIntVal();
  pgrobot->setCBstatus(blast_id, blast_status, this->cbt);
}





CallbackExecute::CallbackExecute(acpObject *b, grobot *g) {
  // constructor
  
  behavior = b;
  pgrobot = g;
}



CallbackExecute::~CallbackExecute() {
  // destructor
  
  // DO WHAT?
}



aErr CallbackExecute::call() {
  // call execution callback
  blast_id = behavior->getNamedValue("unique-id")->getIntVal();
  pgrobot->setCBexec(blast_id);
  
}


