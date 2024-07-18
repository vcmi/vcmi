# Boat

``` javascript
{
	// Layer on which this boat moves. Possible values:
	// "land" - same rules as movement of hero on land
	// "sail" - object can move on water and interact with objects on water
	// "water" - object can walk on water but can not interact with objects
	// "air" - object can fly across any terrain but can not interact with objects
	"layer" : "",
	
	// If set to true, it is possible to attack wandering monsters from boat
	"onboardAssaultAllowed" : true;
	
	// If set to true, it is possible to visit object (e.g. pick up resources) from boat
	"onboardVisitAllowed" : true;
	
	// Path to file that contains animated boat movement
	"actualAnimation" : "",
	
	// Path to file that contains animated overlay animation, such as waves around boat
	"overlayAnimation" : "",
	
	// Path to 8 files that contain animated flag of the boat. 8 files, one per each player
	"flagAnimations" : ["", "" ],
	
	// List of bonuses that will be granted to hero located in the boat
	"bonuses" : { BONUS_FORMAT }
}
```