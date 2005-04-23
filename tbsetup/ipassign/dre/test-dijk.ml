(*
 * test-dijk.ml
 * Test functions for my Dijkstra's shortest path implementation
 *)

(* Hmm, this is awkward, I have to declare all of these types even though they
 * are related. There's probably some better way to do this *)
type mygraph = (int, int) Graph.t;;
type mynode = (int, int) Graph.node;;
type myedge = (int, int) Graph.edge;;

type edge = int * int;;

(* Get a list of edges from a channel *)
let rec (get_edges : in_channel -> edge list) = function channel ->
    try
        let line = input_line channel in
        let parts = Str.split (Str.regexp " +") line in
        (int_of_string (List.nth parts 2), int_of_string (List.nth parts 3))
            :: get_edges channel
    with
        End_of_file -> []
;;

(* Read in one of Jon's graph files *)
let (read_graph_file : string -> edge list) = function filename ->
    let channel = open_in filename in
    get_edges channel
;;

(* Make a graph from and edge_list *)
let rec (make_graph_from_edges : edge list -> mygraph) = function edges ->
    match edges with
      [] -> Graph.empty_graph()
    | x::xs -> let g = make_graph_from_edges xs in
        (match x with (first, second) -> 
            (* Add the verticies to the graph if they are not in there
             * already *)
            let src =
                if not (Graph.is_member g first) then Graph.add_node g first
                    else Graph.find_node g first in
            let dst =
                if not (Graph.is_member g second) then Graph.add_node g second
                    else Graph.find_node g second in
            let edge = Graph.add_edge g src dst 1 in
        g)
;;

let rec print_weights (channel : out_channel) (weights : int array)
                      (index : int) : unit =
    if index >= Array.length weights then ()
    else (output_string channel (string_of_int index ^ ": " ^ string_of_int
        weights.(index) ^ "\n");
        print_weights channel weights (index + 1))
;;

let rec print_pred (channel : out_channel) (preds : (int, 'b) Graph.node array)
                   (index : int) : unit =
    if index >= Array.length preds then ()
    else (output_string channel (string_of_int index ^ ": " ^ string_of_int
        preds.(index).Graph.node_contents ^ "\n");
        print_pred channel preds (index + 1))
;;

let rec print_fhop (channel : out_channel) (fhops : (int, 'a) Dijkstra.first_hop array)
                   (index : int) : unit =
    if index >= Array.length fhops then ()
    else (output_string channel (string_of_int index ^ ": " ^ (match fhops.(index) with
      Dijkstra.NoHop -> "X"
    | Dijkstra.NodeHop(x) -> (string_of_int x.Graph.node_contents)) ^ "\n");
    print_fhop channel fhops (index + 1))
;;

let rec dijk_all_nodes (g : ('a, 'b) Graph.t) (nodes : ('a, 'b) Graph.node list)
: unit =
    match nodes with
      [] -> ()
    | (x :: xs) ->
            (* print_endline ("On " ^ string_of_int x.Graph.node_contents); *)
            match Dijkstra.run_dijkstra g x with (_,pred) ->
            (* XXX - return this somehow *)
            let fhops = Dijkstra.get_first_hops g pred x in
            print_endline ("FHops for " ^ (string_of_int x.Graph.node_contents));
            print_fhop stdout fhops 0;
            (*
            let res = Dijkstra.run_dijkstra g x in
            match res with (weights,_) -> print_weights stdout weights 0; *)
            dijk_all_nodes g xs
;;

exception NeedArg;;
if Array.length Sys.argv < 2 then raise NeedArg;;
(* print_endline "Here 1"; *)

let edges = read_graph_file Sys.argv.(1) in
(* print_endline "Here 2"; *)
let g = make_graph_from_edges edges in
(* print_endline "Here 3"; *)
let node = Graph.find_node g 0 in
(* print_endline "Here 4"; *)
let _ = dijk_all_nodes g g.Graph.nodes in
();
(* print_endline "Here 5"; *)
(*
let res = Dijkstra.run_dijkstra g node in
match res with (weights,preds) ->
(* print_weights stdout weights 0; *)
print_pred stdout preds 0;;
*)
