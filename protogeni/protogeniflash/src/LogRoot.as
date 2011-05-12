package
{
  public interface LogRoot
  {
	function appendMessage(msg : LogMessage):void;
	function setStatus(name:String, isError:Boolean):void;
    function clear():void;
	function open():void;
	function openGroup(id:String):void;
  }
}
