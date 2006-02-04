(*
 * tree-assign.ml
 * Assign IP addresses to a tree
 *)

(* Read in the graph file *)
exception NotATree;;
let g = Graph.read_subgraph_file Sys.argv.(1);;
let n_nodes = Graph.count_nodes g;;
(*
let heights = Array.make n_nodes ~-1;;

let leaves = List.fold_left
                (* (fun l n -> if List.length n.Graph.node_edges == 1 then
                                n :: l
                            else l) *)
                (fun l n -> print_endline ((string_of_int n.Graph.incident_edges) ^ " incident edges");
                            if n.Graph.incident_edges == 1 then
                                n :: l
                            else l)
                [] g.Graph.nodes;;

let heap = Heap.make_heap (Graph.empty_node ~-1);;
List.iter (fun x -> (heights.(x.Graph.node_contents) <- 0; let _ = Heap.insert heap 0 x  in ())) leaves;;

(*
let pick_parent (node : ('a, 'b) Graph.node) : ('a, 'b) Graph.node option =
    (* XXX: Check for tree-ness! *)
    let rec helper (edges : ('a,'b) Graph.edge_list) : ('a,'b) Graph.node option =
        match edges with
          [] -> None (* NotATree *)
        | x :: xs -> let dst = if x.Graph.src == node then x.Graph.dst else
                                                           x.Graph.src in
                if heights.(dst.Graph.node_contents) = ~-1 then Some(dst)
                        else helper xs in
    helper node.Graph.node_edges
;;
*)
let get_parents (node : ('a, 'b) Graph.node) : ('a, 'b) Graph.node list =
    (* XXX: Check for tree-ness! *)
    let rec helper (edges : ('a,'b) Graph.edge_list) : ('a,'b) Graph.node list =
        match edges with
          [] -> [] 
        | x :: xs -> print_endline "helper"; let dst = if x.Graph.src == node then x.Graph.dst else
                                                           x.Graph.src in
                if heights.(dst.Graph.node_contents) = ~-1 then
                    (print_endline "cat"; dst :: helper xs)
                else (print_endline "nocat"; helper xs) in
    helper node.Graph.node_edges
;;

(* Set the heights of all nodes in the tree *)
(*
let rec set_heights () : unit =
    if (Heap.size heap) = 0 then () else begin
        let (height, node) = Heap.min heap in
        Heap.extract_min heap;
        let parent = pick_parent node in
        (match parent with
          None -> ()
        | Some(p) ->
            let newheight = height + 1 in
            heights.(p.Graph.node_contents) <- newheight;
            let _ = Heap.insert heap newheight p in ());
        set_heights()
    end
in set_heights();;
*)
let rec set_heights () : unit =
    if (Heap.size heap) = 0 then () else begin
        let (height, node) = Heap.min heap in
        Heap.extract_min heap;
        let parents = get_parents node in
        print_endline ("nparents = " ^ (string_of_int (List.length parents)));
        let rec mark_parents parents =
            match parents with
              [] -> ()
            | p :: xs ->
                let newheight = height + 1 in
                print_endline ("heights[" ^ (string_of_int
                p.Graph.node_contents) ^ "] = " ^ (string_of_int newheight));
                heights.(p.Graph.node_contents) <- newheight;
                let _ = Heap.insert heap newheight p in
                mark_parents xs
        in
        mark_parents parents;
        set_heights()
    end
in set_heights();;

(* Find the maximum height, and call that the root *)
let max_height = ref ~-1;;
let root = ref ~-1;;
Array.iteri (fun i x -> if x > !max_height then (max_height := x; root := i)
    else ()) heights;;
*)

let min_depth = ref max_int;;
let best_root = ref ~-1;;

let rec tree_depth (root : ('a, 'b) Graph.node) : int = 
    let visited = Array.make n_nodes false in
    let rec helper (root : ('a, 'b) Graph.node) : int =
        visited.(root.Graph.node_contents) <- true;
        let other_end node edge =
            if edge.Graph.src == node then edge.Graph.dst else edge.Graph.src in
        let neighbors = List.map (other_end root) root.Graph.node_edges in
        let filter x = 
            (not visited.(x.Graph.node_contents)) && 
            (not (x == root))
        in
        let (children,_) = List.partition filter neighbors in
        let mark (node : ('a, 'b) Graph.node) : unit =
            visited.(node.Graph.node_contents) <- true
        in
        List.iter mark children;
        let child_depths = List.map helper children in
        let sorted_depths = List.fast_sort (fun a b -> compare b a) child_depths in
        match sorted_depths with
          [] -> 0
        | x :: _ -> (* x + 1 *) (* XXX *)
            let nnames = (List.length children) + 1 in
            let flog = (log (float_of_int nnames)) /. (log 2.0) in
            let nbits = int_of_float (ceil flog) in
            x + nbits
    in
    helper root
;;

let check_root root =
    let depth = tree_depth root in
    if depth < !min_depth then begin
        min_depth := depth;
        best_root := root.Graph.node_contents
    end
;;

List.iter check_root g.Graph.nodes;;

(*
print_endline ("Root is " ^ (string_of_int !best_root) ^ ", depth " ^
                 (string_of_int !min_depth));;
*)

(* Now traverse from the root down, naming things all willy-nilly like *)
let names = Array.make n_nodes Int32.minus_one;;
let rec name_node (bitssofar : int) (node : ('a, 'b) Graph.node) : int =
    let othernodes = List.map
        (fun x -> if x.Graph.src == node then x.Graph.dst else x.Graph.src)
        node.Graph.node_edges in
    let (children,parents) = List.partition (fun (x: ('a, 'b) Graph.node) -> 
            if names.(x.Graph.node_contents) = Int32.minus_one then true
                else false)
        othernodes in
    (* Do some sanity checking on this tree *)
    (* if (List.length parents) > 1 then raise NotATree; *)
    let nnames = (List.length children) + 1 in
    let flog = (log (float_of_int nnames)) /. (log 2.0) in
    let nbits = int_of_float (ceil flog) in
    (* I was named by my parent *)
    let myname = names.(node.Graph.node_contents) in
    (* Name my children *)
    let rec name_children (name : int) (children : ('a, 'b) Graph.node list) : unit =
        match children with
          [] -> ()
        | x :: xs ->
                (names.(x.Graph.node_contents) <- Int32.logor myname
                    (Int32.shift_left (Int32.of_int name) (32 - bitssofar - nbits));
                 name_children (name + 1) xs) in
    name_children 1 children;
    let childheights = List.map (name_node (nbits + bitssofar)) children in
    let sortedheights = List.fast_sort (fun a b -> compare b a) childheights in
    let bigkid = match sortedheights with
                   [] -> 0
                 | x :: _ -> x in
    nbits + bigkid
;;

names.(!best_root) <- Int32.zero;;
let nbits = name_node 0 (Graph.find_node g !best_root);;
print_endline ("bits " ^ (string_of_int nbits));;
print_endline "routes 0";;
Array.iter (fun x -> Printf.printf "%lu\n" (Int32.shift_right_logical x (32 - nbits))) names;;
