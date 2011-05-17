package protogeni.resources
{
	public interface RspecProcessorInterface
	{
		// Process
		function processResourceRspec(afterCompletion : Function):void;
		function processSliverRspec(s:Sliver):void;
		
		// Generate
		function generateSliverRspec(s:Sliver):XML;
	}
}