(*
 * heap.ml - simple heap module
 *)
(* Types used by this module *)
type location = int;;
type weight = int;;
type weight_update_function = weight -> unit;;
type 'a heap_remove_function = 'a -> unit;;
let inf_weight = 32000;;

type 'a heap_data = { mutable key : weight;
                      value : 'a;
                      mutable loc : location };;

type 'a heap = { mutable arr : 'a heap_data array;
                 mutable len : int;
                 def : 'a };;

let make_heap (default : 'a) : 'a heap =
    { arr = Array.make 1 { key = 0; value = default; loc = 0 };
      len = 0;
      def = default };;

let parent (i : int) : int =
    (i - 1) / 2;;

let left (i : int) : int =
    (i * 2) + 1;;

let right (i : int) : int =
    (i * 2) + 2;;

exception EmptyHeap
exception BadIndex
let rec heapify_up (heap : 'a heap) (data : 'a heap_data) (i : location)
        : unit =
    if i >= heap.len then raise BadIndex;
    if i <= 0 || heap.arr.(parent i).key <= data.key then
        (heap.arr.(i) <- data;
        heap.arr.(i).loc <- i)
    else
        (heap.arr.(i) <- heap.arr.(parent i);
        heap.arr.(i).loc <- i;
        heapify_up heap data (parent i));;

let rec heapify (heap : 'a heap) (i : location) : unit =
    let l = left i in
    let r = right i in
    let largest =
        if l <= heap.len && heap.arr.(l).key < heap.arr.(i).key
        then l else i
    in
    let largest =
        if r <= heap.len && heap.arr.(r).key < heap.arr.(largest).key
        then r else largest
    in
    if largest != i then
        (let tmp = heap.arr.(i) in
        heap.arr.(i) <- heap.arr.(largest);
        heap.arr.(largest) <- tmp;
        heap.arr.(i).loc <- i;
        heap.arr.(largest).loc <- largest;
        heapify heap largest)
    else
        ();;

let extract (heap : 'a heap) (index : int) : unit =
    if index < 0 then raise BadIndex else
    if index >= heap.len then raise BadIndex else
    if heap.len < 1 then raise EmptyHeap else
        heap.len <- heap.len - 1;
        if index != heap.len then begin
        heap.arr.(index) <- heap.arr.(heap.len);
        heap.arr.(index).loc <- index;
        heapify heap index end;;

let extract_element (heap : 'a heap) (data : 'a heap_data) (x : 'b) : unit =
    (* print_endline ("Extracting element at " ^ (string_of_int data.loc)); *)
    extract heap data.loc
;;

let update_weight (heap : 'a heap) (data : 'a heap_data) (weight : weight)
        : unit =
    data.key <- weight;
    heapify_up heap data data.loc;;

let insert_obj (heap : 'a heap) (key : weight) (value : 'a)
        : ('a heap_data) =
    if (Array.length heap.arr) = heap.len then
        (let narr = Array.make ((Array.length heap.arr) * 2)
            { key = 0; value = heap.def; loc = 0 } in
        Array.blit heap.arr 0 narr 0 heap.len;
        heap.arr <- narr);
    let newdata = { key = key; value = value; loc = heap.len } in
    Array.set heap.arr heap.len newdata;
    heap.len <- heap.len + 1;
    update_weight heap newdata key;
    newdata;;

let insert (heap : 'a heap) (key : weight) (value : 'a)
        : weight_update_function =
    (* Copy array to grow it if necessary *)
    let newdata = insert_obj heap key value in
    fun w -> update_weight heap newdata w;;

let insert_remove (heap : 'a heap) (key : int) (value : 'a)
        : ('b -> unit)  =
    (* Copy array to grow it if necessary *)
    let newdata = insert_obj heap key value in
    fun x -> extract_element heap newdata x;;

let extract_min (heap : 'a heap) : unit =
    extract heap 0;;

let min (heap : 'a heap) : (weight * 'a) =
    if heap.len = 0 then raise EmptyHeap else
        let record = Array.get heap.arr 0 in
        (record.key, record.value);;

let iter (heap : ' heap) (visitor : ('a -> unit)) : unit =
    let rec heap_iter (i : int) : unit =
        if i >= heap.len then ()
        else (visitor (Array.get heap.arr i).value; heap_iter (i + 1))
    in
    heap_iter 0
;;

let iterw (heap : ' heap) (visitor : (int -> 'a -> unit)) : unit =
    let rec heap_iter (i : int) : unit =
        if i >= heap.len then ()
        else (visitor (Array.get heap.arr i).key (Array.get heap.arr i).value; heap_iter (i + 1))
    in
    heap_iter 0
;;

let size (heap : 'a heap) : int =
    heap.len;;
