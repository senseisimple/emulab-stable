package
{
  public interface LogRoot
  {
    function appendText(newText : String) : void;		// Depreciated
	function appendMessage(msg : LogMessage) : void;
    function clear() : void;
  }
}
